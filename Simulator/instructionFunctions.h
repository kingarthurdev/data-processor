// instruction handlers
void and(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void or(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void xor(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void not(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void shftr(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void shftri(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void shftl(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void shftli(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void br(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void brr_reg(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void brr_lit(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void brnz(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void call(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void run_return(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void brgt(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void priv(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void mov_load(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void mov_reg(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void mov_lit(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void mov_store(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void addf(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void subf(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void mulf(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void divf(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void add(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void addi(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void sub(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void subi(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void mul(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
void run_div(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);

//one more for setting statics:
void setStatics(uint64_t *registers, unsigned char* programMemory,uint64_t *programCounter);
