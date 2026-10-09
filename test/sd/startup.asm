        .module startup
        .globl  _main

        .area HOME (CODE)
__reset_vector:
        ljmp __startup

        .area CSEG (CODE)
__startup:
        mov sp, #0x2f
        lcall _main

__halt:
        sjmp __halt
