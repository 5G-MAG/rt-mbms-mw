#include "File.h"
#include <iostream>
#include <string>
#include <cmath>
#include <cassert>
#include <algorithm>

LibFlute::File::File(const LibFlute::FileDeliveryTable::FileEntry& entry)
  : _fec_oti( entry.fec_oti )
  , _meta( entry )
  , _received_at( time(nullptr) )
{
  // Calculate source block partitioning (RFC5052 9.1) 
  _nof_source_symbols = ceil((double)_fec_oti.transfer_length / (double)_fec_oti.encoding_symbol_length);
  _nof_source_blocks = ceil((double)_nof_source_symbols / (double)_fec_oti.max_source_block_length);
  _large_source_block_length = ceil((double)_nof_source_symbols / (double)_nof_source_blocks);
  _small_source_block_length = floor((double)_nof_source_symbols / (double)_nof_source_blocks);
  _nof_large_source_blocks = _nof_source_symbols - _small_source_block_length * _nof_source_blocks;

  // Allocate a data buffer
  _buffer = (char*)malloc(_fec_oti.transfer_length);
  if (_buffer == nullptr)
  {
    throw "Failed to allocate file buffer";
  }
  auto buffer_ptr = _buffer;

  // Create the required source blocks and encoding symbols
  size_t remaining_size = _fec_oti.transfer_length;
  auto number = 0;
  while (remaining_size > 0) {
    SourceBlock block;
    auto symbol_id = 0;
    auto block_length = ( number < _nof_large_source_blocks ) ? _large_source_block_length : _small_source_block_length;

    for (int i = 0; i < block_length; i++) {
      auto symbol_length = std::min(remaining_size, (size_t)_fec_oti.encoding_symbol_length);
      assert(buffer_ptr + symbol_length <= _buffer + _fec_oti.transfer_length);

      SourceBlock::Symbol symbol{.data = buffer_ptr, .length = symbol_length, .complete = false};
      block.symbols[ symbol_id++ ] = symbol;
      
      remaining_size -= symbol_length;
      buffer_ptr += symbol_length;
      
      if (remaining_size <= 0) break;
    }
    _source_blocks[number++] = block;
  }
}

LibFlute::File::~File()
{
  if (_buffer != nullptr)
  {
    free(_buffer);
  }
}

auto LibFlute::File::put_symbol( const LibFlute::EncodingSymbol& symbol ) -> void
{
  if (symbol.source_block_number() > _source_blocks.size()) {
    throw "Source Block number too high";
  } 

  SourceBlock& source_block = _source_blocks[ symbol.source_block_number() ];
  
  if (symbol.id() > source_block.symbols.size()) {
    throw "Encoding Symbol ID too high";
  } 

  SourceBlock::Symbol& target_symbol = source_block.symbols[symbol.id()];

  if (!target_symbol.complete) {
    symbol.decode_to(target_symbol.data, target_symbol.length);
    target_symbol.complete = true;

    check_source_block_completion(source_block);
    check_file_completion();
  }

}

auto LibFlute::File::check_source_block_completion( SourceBlock& block ) -> void
{
  block.complete = std::all_of(block.symbols.begin(), block.symbols.end(), [](const auto& symbol){ return symbol.second.complete; });
}

auto LibFlute::File::check_file_completion() -> void
{
  _complete = std::all_of(_source_blocks.begin(), _source_blocks.end(), [](const auto& block){ return block.second.complete; });
}
