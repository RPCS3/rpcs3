
         .global defaultEH
         .ent defaultEH
defaultEH:
         .set noreorder
         .set noat
         .word 0
         .word 0

         jal defaultEHfunc
         nop
         cop0 0x10
         nop
         .end defaultEH
         

