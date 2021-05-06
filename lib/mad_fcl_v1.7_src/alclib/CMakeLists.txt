add_compile_options( -DLINUX -D_LARGE_FILE_API -D_LARGEFILE64_SOURCE -D_ISOC99_SOURCE -D_XOPEN_SOURCE=500 -std=c99 -D_GNU_SOURCE)
ADD_LIBRARY( alclib STATIC
  alc_channel.c alc_hdr.c alc_rx.c alc_session.c alc_socket.c alc_tx.c 
  lct_hdr.c mad.c transport.c blocking_alg.c null_fec.c xor_fec.c fec.c rs_fec.c 
  mad_rlc.c alc_list.c )
