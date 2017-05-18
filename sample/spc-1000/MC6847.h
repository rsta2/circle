#ifndef __MC6847_H__
#define __MC6847_H__

void InitMC6847(Uint8 *emul, unsigned char* in_VRAM, int w, int h);
void Update6847(Uint8 gmode);

#endif // __MC6847_H__
