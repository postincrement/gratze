if(NOT DEFINED Z80ASM OR NOT DEFINED ASM OR NOT DEFINED OUT)
  message(FATAL_ERROR "Z80ASM, ASM, and OUT are required")
endif()

get_filename_component(outdir "${OUT}" DIRECTORY)
file(MAKE_DIRECTORY "${outdir}")
set(bin "${outdir}/newbdos.bin")

execute_process(
  COMMAND "${Z80ASM}" -o "${bin}" "${ASM}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE asm_out
  ERROR_VARIABLE asm_err
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "z80asm failed (${rc}): ${asm_out}${asm_err}")
endif()

file(READ "${bin}" hex HEX)
string(LENGTH "${hex}" hexlen)
math(EXPR nbytes "${hexlen} / 2")
if(nbytes LESS 1)
  message(FATAL_ERROR "newbdos image is empty")
endif()

# Image must fit in CCP+BDOS+BIOS (0x800+0x100+0x100 = 0xA00).
if(nbytes GREATER 2560)
  message(FATAL_ERROR "newbdos image is ${nbytes} bytes; max is 2560")
endif()

set(body "unsigned char z80_cpmhost_newbdos_bin[${nbytes}] = {\n")
set(i 0)
while(i LESS nbytes)
  if(i GREATER 0)
    string(APPEND body ",")
    math(EXPR col "${i} % 12")
    if(col EQUAL 0)
      string(APPEND body "\n")
    else()
      string(APPEND body " ")
    endif()
  endif()
  math(EXPR pos "${i} * 2")
  string(SUBSTRING "${hex}" ${pos} 2 byte)
  string(APPEND body "0x${byte}")
  math(EXPR i "${i} + 1")
endwhile()
string(APPEND body "\n};\n")
string(APPEND body "unsigned z80_cpmhost_newbdos_bin_len = ${nbytes};\n")
file(WRITE "${OUT}" "${body}")
