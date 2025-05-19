   .def setASP
   .def setPSP
   .def getPSP
   .def getMSP
   .def UnprivilegedMode
   .def RestoreCurrentTask
   .def SaveTask
   .def get_arg0

.thumb
.const

.text
setASP:
	MRS R3,CONTROL
   	ORR R3,R3,#0x02
   	MSR CONTROL,R3
   	ISB
   	BX LR
setPSP:
	MSR PSP,R0
	ISB
	BX LR

getPSP:
	MRS R0,PSP
	ISB
	BX LR
getMSP:
	MRS R0,MSP
	ISB
	BX LR
UnprivilegedMode:
	MRS R3,CONTROL
	ORR R3,R3,#0x01
	MSR CONTROL,R3
	ISB
	BX LR

RestoreCurrentTask:
	MRS R0,PSP
    LDR LR,[R0]
    ADD R0,R0,#4
	LDR R11,[R0]
    ADD R0,R0,#4
    LDR R10,[R0]
    ADD R0,R0,#4
    LDR R9,[R0]
    ADD R0,R0,#4
    LDR R8,[R0]
    ADD R0,R0,#4
    LDR R7,[R0]
    ADD R0,R0,#4
    LDR R6,[R0]
    ADD R0,R0,#4
    LDR R5,[R0]
    ADD R0,R0,#4
    LDR R4,[R0]
    ADD R0,R0,#4
    MSR PSP,R0
    BX LR
SaveTask:
	MRS R0,PSP
    SUB R0,R0,#4
    STR R4,[R0]
    SUB R0,R0,#4
    STR R5,[R0]
    SUB R0,R0,#4
    STR R6,[R0]
    SUB R0,R0,#4
    STR R7,[R0]
    SUB R0,R0,#4
    STR R8,[R0]
    SUB R0,R0,#4
    STR R9,[R0]
    SUB R0,R0,#4
    STR R10,[R0]
    SUB R0,R0,#4
    STR R11,[R0]
    SUB R0,R0,#4
    MOVW R1,#0xFFFD
    MOVT R1,#0xFFFF
    STR R1,[R0]
    MSR PSP,R0
    BX LR

get_arg0:
	MRS R0,PSP
    LDR R0,[R0]
    BX LR
