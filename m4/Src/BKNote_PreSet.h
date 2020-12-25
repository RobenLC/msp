//the original point (the first pixel) of BMP image is at lower left, if the image size is postive.
//the original point (the first pixel) of BMP image is at Top left, if the image size is negative.
#define	BMP_Image_Size_Offset		0x22			//signed integer, 4 bytes
#define	BMP_Width_Offset			0x12			//signed integer, 4 bytes
#define	BMP_Height_Offset			0x16			//signed integer
#define BMP_IMG_1stPxl_Ptr			0x0A			//4 bytes
#define BMP_IMG_FileSize_Offset		0x02			//4 bytes

#define FaceSideNote	0
#define BackSideNote	1

#define LeftSideNote	0
#define RightSideNote	1

#define	NTD100Img	1
#define	NTD200Img	2
#define	NTD500Img	3
#define	NTD1000Img	4
#define	NTD2000Img	5
#define NTDBill_Height		540		//a constant for NTD only

#define	NTD100LED	GREENLED
#define	NTD200LED	GREENLED
#define	NTD500LED	BLUELED
#define	NTD1000LED	BLUELED
#define	NTD2000LED	BLUELED


#define	RMB1Img	1
#define	RMB5Img	2
#define	RMB10Img	3
#define	RMB20Img	4
#define	RMB50Img	5
#define	RMB100Img	6
#define	RMB100NewImgH	7
#define	RMB100NewImgV	8

#define	NTD_CHKP_Face	1
#define	NTD_CHKP_Back	2
#define	RMB_CHKP_Face	3
#define	RMB_CHKP_Back	4

/*
#define	NTD100B_BillN	1
#define	NTD200B_BillN	2
#define	NTD500B_BillN	3
#define	NTD1000B_BillN	4
#define	NTD2000B_BillN	5
*/

typedef  struct  {
	int posx;            //Point 
	int posy;             
	int	BillNum;
}	Pos_XY;

typedef  struct  {
	int Char_Num;             	//Number of char in the block
	int *VRT_Edges;            	//vertical boundary pairs 
	int *HORZ_Edges;            //Horizontal boundary pairs
	unsigned char VRT_Pairs;   
	unsigned char HORZ_Pairs;   
	unsigned char  Max_Char;    //Max allowed char in the array, the allocated array size is two times of Max_Char +1
	unsigned char  Char_VRT;    //direction of char verical=1 or horizontal=0
}	AlphaB_BlockSet;

typedef  struct  {
	unsigned char ABwidth;             		//18
	unsigned char ABheight;            		//27
	unsigned char ABwidth_Min;            	//6
	unsigned char ABheight_Min;           	//9 
}	AlphaBlk_Param;


typedef  struct  {
	int ABwidth;             	//18
	int ABheight;            	//27
	int ABwidth_Min;            //6
	int ABheight_Min;           //9 
}	AlphaB_PSet;

typedef  struct  { 
	int  Chk_Left_Offfet;	//LX, CHK block, Left offset         //48	 
	int  Chk_L_BTM_Offset;	//LY, CHK block, Bottom offset 		//200				
	int  Chk_Right_Offfet;	//RX, CHK block, Right offset        //48	 
	int  Chk_R_BTM_Offfet;	//RY, CHK block, Top offset			//440			
	int  Chk_Block_Width;	//CHK block, Width  //48	
	int  Chk_Block_Height;	//CHK block, Height //118	
}	BKNote_ChkP_Parameter;

/*
struct BKNote_ChkP_Parameter { 
	int  Chk_TOP_Offfet;				//CHK block, Top offset if Original Point at Top left			//440			
	int  Chk_BTM_Offset;				//CHK block, Bottom offset if Original Point at bottom left		//200				
	int  Chk_Left_Offfet;				//CHK block, Left offset                                        //48	 
	int  Chk_Right_Offfet;			//CHK block, Right offset                                        //48	 
	int  Chk_Block_Width;				//CHK block, Width  //48	
	int  Chk_Block_Height;			//CHK block, Height //118	
	int  Chk_CenterP_Height_100;		//Center Point, Height   
	int  Chk_CenterP_Left_100;			//Center Point, Left   
	int  Chk_CenterP_Height_500;		//Center Point, Height   
	int  Chk_CenterP_Left_500;			//Center Point, Left   
	int  Chk_CenterP_Height_1000;		//Center Point, Height   
	int  Chk_CenterP_Left_1000;			//Center Point, Left   
};
*/
//From ChK Point
/*
struct BKNote_SRN_Parameter {
  int  SRN_Left_OrgX;				//LEFT serial number block, Org_X   
  int  SRN_Left_OrgY;			    //LEFT serial number block, Org_Y	
  int  SRN_Right_OrgX;				//Right serial number block, Org_X      	
  int  SRN_Right_OrgY;				//Right serial number block, Org_Y					
  int  SRN_Block_Width;				//serial number block, Width       	
  int  SRN_Block_Height;			//serial number block, Height      	
};
*/

//From ChK Point, original point is locate at lower_left, image is on Phase one
typedef  struct  {
      int  IMGBlk_Left_OrgX;				//LEFT image block, Org_X   
      int  IMGBlk_Left_OrgY;			    //LEFT image block, Org_Y	
      int  IMGBlk_Right_OrgX;				//Right Image block, Org_X      	
      int  IMGBlk_Right_OrgY;				//Right Image block, Org_Y					
      int  IMGBlk_Width;				    //Image Width       	
      int  IMGBlk_Height;			        //Image Height      	
}	BKNote_ImgBlk_Param;

/*
struct BKNote_SRN_Parameter {
  int  SRN_TOP_Offfet;				//serial number block, Top offset if Original Point at Top left			//440			
  int  SRN_BTM_Offset;				//serial number block, Bottom offset if Original Point at bottom left	//110					
  int  SRN_Left_Offfet;				//serial number block, Left offset                                      //52	
  int  SRN_Right_Offfet;			//serial number block, Right offset                                      //52	
  int  SRN_Block_Width;				//serial number block, Width                                            //240	
  int  SRN_Block_Height;			//serial number block, Height                                           //60	
//    SRN_Block_Size;				//SRN_Block_Width * SRN_Block_Height	
};
*/
//struct Imgheader {
//	int yr, xc;             /* Rows and columns in the image */
//	int oyi, oxj;             /* Origin */
//};

typedef  struct  {
  int 	posx; 			//Org_X, offset in whole image
  int 	posy; 			//Org_Y, offset in whole image
  int  	Block_Width;			//nc, block Width    
  int  	Block_Height;			//nr, block Height
}	imgBlock_Loc_Set;

//typedef struct BKNote_ChkP_Parameter *BKNote_ChkP_Ptr;
//typedef struct BKNote_SRN_Parameter* BKNote_SRN_Ptr;
//typedef struct BKNote_ImgBlk_Param *BKNote_ImgBlk_Ptr;
//typedef struct imgBlock_Loc_Set *imgBlock_LocPtr;
//typedef struct AlphaB_BlockSet *AlphaB_Block_Ptr;
typedef struct Pos_XY *POS_XY_Ptr;

//Pos_XY ChK_Point_NTD(IMAGEIP_Ptr SrcImage);
AlphaB_BlockSet *Allocate_AlphaB_BlockSet(unsigned char Max_Char);

BKNote_ImgBlk_Param *Get_RMB_BKNote_SNR_Param(int BillNumber);
//BKNote_SRN_Parameter *Get_NTD_BKNote_SNR_Param(int BillNumber);
BKNote_ImgBlk_Param *Get_NTD_BKNote_SNR_Param(int BillNumber);
BKNote_ImgBlk_Param *Get_NTD_BKNote_BillN_Param(int BillNumber);
BKNote_ChkP_Parameter *Get_NTD_CHK_LOC_Param(int BillNumber);

int CHK_LowestLine(imageIP *TGTImage, int StartLine, int Match_PixelNum, int MatchLines, unsigned char MarkV);
int CHK_HighestLine(imageIP *TGTImage, int StartLine, int Match_PixelNum, int MatchLines, unsigned char MarkV);
int CHK_LeftLine(imageIP *TGTImage, int StartLine, int Match_PixelNum, unsigned char MarkV);
int CHK_RightLine(imageIP *TGTImage, int StartLine, int Match_PixelNum, unsigned char MarkV);


int Find_HighEdge(imageIP * TGTImage, ImgRectC *ChkBlock, int MinPixels, int MinLines, unsigned char MarkV);
int Find_LowEdge(imageIP * TGTImage, ImgRectC *ChkBlock, int MinPixels, int MinLines, unsigned char MarkV);
int Find_LeftEdge(imageIP * TGTImage, ImgRectC *ChkBlock, int MinPixels, int MinLines, unsigned char MarkV);
int Find_RightEdge(imageIP * TGTImage, ImgRectC *ChkBlock, int MinPixels, int MinLines, unsigned char MarkV);


void CHK_WHT_VRT_Gap(imageIP *TGTImage, AlphaB_BlockSet *OCRImg_Blk, int HighLine, int LowLine, int Minimun_Gap_Width);
int CHK_HighBlkLine(imageIP *TGTImage, int Black_PixelNum, int Continue_Lines);
int CHK_LowBlkLine(imageIP *TGTImage, int Black_PixelNum, int Continue_Lines);
void Set_IndAlphaB_ImgBlk(imageIP *TGTImage, AlphaB_BlockSet *OCRImg_Blk, AlphaB_PSet *TAlphaB_PSet, int HighLine, int LowLine);
int Get_WHT_HOZ_Gap_Num(imageIP *TGTImage, imgBlock_Loc_Set *TImg_Box, int Minimun_Gap_Width,  int Max_Gap_Num);
void VRT_Center_Ana(int *PixelCountArray, int *VRTCntrArray, int xc);	//nc, 228, 240
void freeAlphaB_BlockSet(AlphaB_BlockSet *z);


