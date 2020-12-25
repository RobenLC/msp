

void Convol3x3(unsigned char *XImg, int yyr, int xc, int *KElement);
void dil_apply(imageIP *im, SElement *p, int yi, int xj, imageIP *res);
int bin_dilate(imageIP *im, SElement *p);
void erode_apply(imageIP *im, SElement *p, int yi, int xj, imageIP *res);
int bin_erode(imageIP *im, SElement *p);
//int CopyByte2Int(unsigned char *SrcPtr);
void Get_3x3SElement(SElement *T_SElement, int SE_Number);
void Get_2x2SElement(SElement *T_SElement, int SE_Number);
void Get_4x4SElement(SElement *T_SElement, int SE_Number);


