#ifndef IPS_H
#define IPS_H

/** ApplyIPS() ***********************************************/
/** Loads patches from an .IPS file and applies them to the **/
/** given data buffer. Returns number of patches applied.   **/
/*************************************************************/
unsigned int ApplyIPS(const char *FileName,unsigned char *Data,unsigned int Size);

/** MeasureIPS() *********************************************/
/** Find total data size assumed by a given .IPS file.      **/
/*************************************************************/
unsigned int MeasureIPS(const char *FileName);

#endif /* IPS_H */
