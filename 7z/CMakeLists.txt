set(LIB_SRCS 
		7zAlloc.c
		7zBuf.c
		7zBuf2.c
		7zCrc.c
		7zCrcOpt.c
		7zDec.c
		7zFile.c
		7zIn.c
		LzmaDec.c
		Lzma2Dec.c
		7zStream.c
		Bcj2.c
		Bra.c
		Bra86.c
		Ppmd7.c
		Ppmd7Dec.c
		CpuArch.c
	)

add_library(7zdec STATIC ${LIB_SRCS})
