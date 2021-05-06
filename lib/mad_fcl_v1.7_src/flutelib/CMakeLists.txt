add_compile_options( -DLINUX -D_LARGE_FILE_API -D_LARGEFILE64_SOURCE -D_ISOC99_SOURCE -D_XOPEN_SOURCE=500 -std=c99 -D_GNU_SOURCE)
ADD_LIBRARY( flutelib STATIC
  flute.c sender.c receiver.c fdt.c fdt_gen.c mad_md5.c uri.c getdnsname.c mad_zlib.c 
	padding_encoding.c parse_args.c mad_utf8.c http_file_repair.c flute_file_repair.c apd.c
	display.c )
target_link_libraries( flutelib alclib sdplib )
