#ifndef ASM_H_
#define ASM_H_

extern void setASP();
extern void setPSP(uint32_t *add);
extern uint32_t getMSP();
extern uint32_t getPSP();
extern void UnprivilegedMode();
extern void RestoreCurrentTask();
extern void SaveTask();
extern uint32_t get_arg0();

#endif /* ASM_H_ */
