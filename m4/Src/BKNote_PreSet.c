	/* Includes ------------------------------------------------------------------*/
#include "../Inc/main.h"
//#include "log.h"
#include "arrayed_queue.h"
#include <string.h>

/*
#include "stdafx.h"
#include <iostream>
#include <windows.h>
*/

/*
	//#include <opencv2/opencv.hpp>
	//#include "opencv2/imgproc.hpp"
	//#include "opencv2/imgcodecs.hpp"
	//#include "opencv2/highgui.hpp"
	
#include "ImageCV_Main.hpp"
#include "ImageCV_Samples.hpp"
	*/
	//#include "morph.h"
	
/*	
#include <stdexcept>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include "CroppingLib.h"
#include <iostream>
#include <string>
#include <windows.h>
#include <Winbase.h>
#include <Pathcch.h>
*/


#include "BasicSet.h"
//#include "BMPFile_Set.h"
#include "Image_PreProcess.h"
#include "CFilters.h"
#include "BKNote_PreSet.h"
//#include "ConsoleBankNote.h"
	
//using namespace std;
//6	
//6
//int  Chk_CenterP_Height;		//Center Point, Height	 
//int  Chk_CenterP_Left;			//Center Point, Left   

#define Match_PixelNum_NTDSRN	5

//20200613: 
//the NTD_ChkF_FLoc is used for chekcing bill number.
//firstly, it check the left side of bill.
//if the SRN and the check mark is on the left side, tehn, the result van be got.
//if not, it will need to check the check mark on the right hand side of bill.
//because the bill size is varied by the bill number, the location of the check box is varied.
//it needs to get the width of current bill, to get the esimate loction of the check bosx on the right end.
//because the height of bill could be truncated, it can not be used to figure out the height value of location.
//however, the NTD bill are all having same height. It will be used to do the calculation and not using the result of cropping.
//#define NTDBill_Height		540

//Height:69.5mm, 69.5/25.4*200=548
//depending on the design of roller and the condition, the height is varied. 
//In one scanner, it is about 535. it is 2.5% difference.

//1000:161mm, 500:156mm, 100:145.5mm, 1000=1268, 500=1228, 100=1146, 2000=1305?, 200=1188
const  BKNote_ChkP_Parameter  NTD_ChkF_FLoc =
{
		40, 		   //Chk_Left_Offfet; X	  
		200,		   //Chk_L_BTM_Offset; Y	  
//		1268-48-72,    //Chk_Right_Offfet;   //need to use scaned width to substract. 1000=1268, 500=1219, 100=1136
		1268-40-80,    //Chk_Right_Offfet; X //need to use scaned width to substract. 1000=1268, 500=1228, 100=1146
		548-200-144,   //Chk_R_BTM_Offfet; Y //1000=530, 500=536, 100=534	
		80, 		   //Chk_Block_Width;	  
		144 		   //Chk_Block_Height;	   
};	

const  BKNote_ChkP_Parameter  NTD_ChkB_BLoc =
{
	  534-200-144,	 		//Chk_TOP_Offfet;		//1000=530, 500=536, 100=534  
	  310-20,			 	//Chk_BTM_Offset;		
	  890-64,			 	//Chk_Left_Offfet; 	
	  1268-48-48,	 		//Chk_Right_Offfet;	//need to use scaned width to substract. 1000=1268, 500=1219, 100=1136
	  128,			 		//Chk_Block_Width; 	
	  80			 		//Chk_Block_Height;	 
};	
/*
const  BKNote_ChkP_Parameter  NTD_ChkB_BLoc =
{
	  534-200-144,	 		//Chk_TOP_Offfet;		//1000=530, 500=536, 100=534  
	  310-20,			 	//Chk_BTM_Offset;		
	  890-64,			 	//Chk_Left_Offfet; 	
	  1268-48-48,	 		//Chk_Right_Offfet;	//need to use scaned width to substract. 1000=1268, 500=1219, 100=1136
	  128,			 		//Chk_Block_Width; 	
	  80			 		//Chk_Block_Height;	 
};	
*/
//from CHK_Point
const  BKNote_ImgBlk_Param  NTD100L_SRN_Param = {
//	   0, 		  		//LEFT image block, Org_X , from cnter of check point      
//	  -12,				 //LEFT image block, Org_X , 20200609		
	   -8,			   //LEFT image block, Org_X , 20200612	  
//	   42, 		    	//LEFT image block, Org_Y	    		
//	   32, 		    	//LEFT image block, Org_Y, 20200604	    		
//	   42, 		    	//LEFT image block, Org_Y, 20200609	    		
	   38, 		    	//LEFT image block, Org_Y, 20200612	    		
	   812,	  			//Right Image block, Org_X      
	   136, 		    //Right Image block, Org_Y						
//	   228, 		  	//Image Width       	        
	   240, 		  	//Image Width       	        
	   48			  	//Image Height      	        
};

const  BKNote_ImgBlk_Param  NTD200L_SRN_Param = {
	   28, 		  		//LEFT image block, Org_X       
	   92, 		    	//LEFT image block, Org_Y	    		
	   840,	  			//Right Image block, Org_X      
	   194, 		    //Right Image block, Org_Y						
	   240, 		  	//Image Width       	        
	   48			  	//Image Height      	        
};


//from CHK_Point
const  BKNote_ImgBlk_Param  NTD500L_SRN_Param = {
//	   80, 		  		//LEFT image block, Org_X       
	   70, 		  		//LEFT image block, Org_X, 20200609       
//	   72, 		    	//LEFT image block, Org_Y	    		
	   70, 		    	//LEFT image block, Org_Y	20200609    		
	   780,	  			//Right Image block, Org_X      
	   170, 		    //Right Image block, Org_Y						
//	   228, 		  	//Image Width       	        
	   240, 		  	//Image Width     //20200609  	        
	   48			  	//Image Height      	        
};
	
//from CHK Point	
const  BKNote_ImgBlk_Param  NTD1000L_SRN_Param = {
//	   92, 		  		//LEFT image block, Org_X       
	   82, 		  		//LEFT image block, Org_X     //20200609   
	   60, 		    	//LEFT image block, Org_Y	    		
	   786,	  			//Right Image block, Org_X      
	   148, 		    //Right Image block, Org_Y						
//	   228, 		  	//Image Width       	        
	   240, 		  	//Image Width  	//20200609     	        
	   48			  	//Image Height      	        
};

const  BKNote_ImgBlk_Param  NTD2000L_SRN_Param = {
	   106, 		  	//LEFT image block, Org_X       
	   66, 		    	//LEFT image block, Org_Y	    		
	   922,	  			//Right Image block, Org_X      
	   148, 		    //Right Image block, Org_Y						
	   240, 		  	//Image Width       	        
	   48			  	//Image Height      	        
};


//?	   
const  BKNote_ImgBlk_Param NTD100R_SRN_Param = {
	   152, 		  //LEFT image block, Org_X     			
//	   330, 		  //LEFT image block, Org_Y	    					
	   345, 		  //Right Image block, Org_X    					
	   70,			  //Right Image block, Org_Y			
	   1216-148,	  //Image Width       	        		
	   240, 		  //Image Height      	        		
	   48			  //SRN_Block_Height;			
};

//?		
const  BKNote_ImgBlk_Param  NTD500R_SRN_Param = {
	   150, 		  //LEFT image block, Org_X         
	   330, 		  //LEFT image block, Org_Y	        		
	   148, 		  //Right Image block, Org_X        
	   1216-148,	  //Right Image block, Org_Y		
	   240, 		  //Image Width       	            
	   48			  //Image Height      	            
};

 //?
const  BKNote_ImgBlk_Param  NTD1000R_SRN_Param = {
	   172, 		  //LEFT image block, Org_X         	
	   330, 		  //LEFT image block, Org_Y	        			
	   160, 		  //Right Image block, Org_X        
	   790,	  		  //Right Image block, Org_Y		
	   240, 		  //Image Width       	            
	   60			  //Image Height      	            
};
	   
//from CHK_Point
const  BKNote_ImgBlk_Param  NTD100B_BillN_Param = {
	   -848, 		  	//LEFT image block, Org_X       
	   85, 		        //LEFT image block, Org_Y	    		
	   -90,	  			//Right Image block, Org_X      
	   -270, 		    //Right Image block, Org_Y						
	   280, 		  	//Image Width       	        
	   130			  	//Image Height      	        
};

const  BKNote_ImgBlk_Param  NTD200B_BillN_Param = {
	   -848, 		  	//LEFT image block, Org_X       
	   85, 		        //LEFT image block, Org_Y	    		
	   -90,	  			//Right Image block, Org_X      
	   -270, 		    //Right Image block, Org_Y						
	   280, 		  	//Image Width       	        
	   130			  	//Image Height      	        
};

//from CHK_Point
const  BKNote_ImgBlk_Param  NTD500B_BillN_Param = {
	   -835, 		  	//LEFT image block, Org_X       
	   108, 		    //LEFT image block, Org_Y	    			
	   -45,	  			//Right Image block, Org_X      	
	   -270, 		    //Right Image block, Org_Y							
	   300, 		  	//Image Width       	        //200
	   120			  	//Image Height      	        //100
};
	
//from CHK Point	
const  BKNote_ImgBlk_Param  NTD1000B_BillN_Param = {
	   -855, 		  	//LEFT image block, Org_X       
	   115, 		    //LEFT image block, Org_Y	     				
	   20,	  			//Right Image block, Org_X      	
	   -280, 		    //Right Image block, Org_Y		 						
	   280, 		  	//Image Width       	           //240	
	   130			  	//Image Height      	          //90	
};
  
const  BKNote_ImgBlk_Param  NTD2000B_BillN_Param = {
	   -855, 		  	//LEFT image block, Org_X       
	   115, 		    //LEFT image block, Org_Y	     				
	   20,	  			//Right Image block, Org_X      	
	   -280, 		    //Right Image block, Org_Y		 						
	   280, 		  	//Image Width       	           //240	
	   130			  	//Image Height      	          //90	
};

const  BKNote_ImgBlk_Param  RMB100_SRN_Param = {
	   150, 		  //LEFT image block, Org_X     			
	   330, 		  //LEFT image block, Org_Y	    					
	   148, 		  //Right Image block, Org_X    		
	   1216-148,	  //Right Image block, Org_Y			
	   240, 		  //Image Width       	        		
	   60			  //Image Height      	        			
};


const  BKNote_ImgBlk_Param  NRMB100H_SRN_Param = {
	   152, 		  //LEFT image block, Org_X     			
	   330, 		  //LEFT image block, Org_Y	    					
	   72,			  //Right Image block, Org_X    		
	   1216-148,	  //Right Image block, Org_Y				
	   240, 		  //Image Width       	        		
	   60			  //Image Height      	        			
};

const BKNote_ImgBlk_Param NRMB100V_SRN_Param = {
	   172, 		  //LEFT image block, Org_X     			
	   330, 		  //LEFT image block, Org_Y	    					
	   160, 		  //Right Image block, Org_X    		
	   1216-148,	  //Right Image block, Org_Y				
	   240, 		  //Image Width       	        		
	   60			  //Image Height      	        			
};

const BKNote_ImgBlk_Param  RMB50_SRN_Param =
{
	   172, 		  //LEFT image block, Org_X     			
	   330, 		  //LEFT image block, Org_Y	    					
	   160, 		  //Right Image block, Org_X    		
	   1216-148,	  //Right Image block, Org_Y			
	   240, 		  //Image Width       	        		
	   60			  //Image Height      	        		
};

const BKNote_ImgBlk_Param  RMB10_SRN_Param =
{
	   172, 		  //LEFT image block, Org_X     			
	   330, 		  //LEFT image block, Org_Y	    					
	   160, 		  //Right Image block, Org_X    		
	   1216-148,	  //Right Image block, Org_Y				
	   240, 		  //Image Width       	        		
	   60			  //Image Height      	        			
};

const BKNote_ImgBlk_Param *RMB_SNR_Param_Table[] =
{
	0,
	&RMB100_SRN_Param,//10
	&RMB100_SRN_Param,//20
	&RMB100_SRN_Param,//50
	&RMB100_SRN_Param,//100
	&NRMB100H_SRN_Param,// new 100 H
	&NRMB100V_SRN_Param //New 100 V
};

const BKNote_ImgBlk_Param *NTD_SNR_Param_Table[] =
{
	0,
	&NTD100L_SRN_Param,
	&NTD200L_SRN_Param,
	&NTD500L_SRN_Param,
	&NTD1000L_SRN_Param,
	&NTD2000L_SRN_Param
};

const BKNote_ImgBlk_Param *NTD_BillN_Param_Table[] =
{
	0,
	&NTD100B_BillN_Param,
	&NTD200B_BillN_Param,
	&NTD500B_BillN_Param,
	&NTD1000B_BillN_Param,
	&NTD2000B_BillN_Param
};

const BKNote_ChkP_Parameter *BKNote_NTD_CHk_Loc_Table[]=
{
	0,
	&NTD_ChkF_FLoc,
	&NTD_ChkF_FLoc,
	&NTD_ChkB_BLoc,
	&NTD_ChkB_BLoc,
	&NTD_ChkB_BLoc,
	&NTD_ChkB_BLoc
};


//BKNote_SRN_Parameter = (BKNote_SRN_Parameter *)
BKNote_ImgBlk_Param *Get_RMB_BKNote_SNR_Param(int BillNumber)
{
	return ((BKNote_ImgBlk_Param *)RMB_SNR_Param_Table[BillNumber]);

}

BKNote_ImgBlk_Param *Get_NTD_BKNote_SNR_Param(int BillNumber)
{
	return ((BKNote_ImgBlk_Param *)NTD_SNR_Param_Table[BillNumber]);

}

BKNote_ImgBlk_Param *Get_NTD_BKNote_BillN_Param(int BillNumber)
{
	return ((BKNote_ImgBlk_Param *)NTD_SNR_Param_Table[BillNumber]);

}

BKNote_ChkP_Parameter *Get_NTD_CHK_LOC_Param(int BoxParamSelect)
{
	return ((BKNote_ChkP_Parameter *)BKNote_NTD_CHk_Loc_Table[BoxParamSelect]);

}
/*
typedef struct  
{
	int yr;				//Height,y, 	// Rows and columns in the image 
	int	xc;             //Width,x
	int oyi; 			//y, 		// Origin 
	int	oxj;             //x, 
}	ImgRectC;
*/
//check white(MarkV) gaps from left to right, just return the number of gaps in the range of ChkBlock
//find edge is used to find the edge of a check box.
//The check box could be surround with black pixel. so, it will check from the direction of left_to_right or right_to_left, 
//top/high_to_bottom/low, or bottom_to_Top/high to find a minimum number of contiunous lines to have a minimum number of MarkV pixels.
//
//int Find_LeftEdge(imageIP * TGTImage, ImgRectC *ChkBlock, int Match_PixelNum, unsigned char MarkV)
int Find_HighEdge(imageIP * TGTImage, ImgRectC *ChkBlock, int MinPixels, int MinLines, unsigned char MarkV)
{

	unsigned char TData;
	int i, j, PixelCount, LineCount;
	
	PixelCount=0;
	LineCount=0;
/*
	log_info("Find_HighEdge: ChkBlock->oyi = %d, oxj = %d, yr = %d, xc = %d \n",
			ChkBlock->oyi,
			ChkBlock->oxj,
			ChkBlock->yr,
			ChkBlock->xc);
*/
	for (i=ChkBlock->oyi; i< ChkBlock->oyi + ChkBlock->yr; i++)		//from Low to high
	{
		if (LineCount >= MinLines)
		{
			LineCount=i-LineCount;
//			log_info("Find_HighEdge: %d \n",LineCount);
			return LineCount;
		}
//		log_info("------------HiLineCount: %d \n",LineCount);
		if (PixelCount < MinPixels)
		{
			LineCount=0;
		}
		PixelCount=0;		
		for (j = ChkBlock->oxj; j < ChkBlock->oxj+ChkBlock->xc; j++) //from left to right
		{
			TData = TGTImage->data[i][j];
			if (TData == MarkV)
				PixelCount++;
			if (PixelCount >= MinPixels)
			{
				LineCount++;
				break;
			}				
		}			
	}
	int k = ChkBlock->oyi+ ChkBlock->yr;
//	log_info("NoFind_HighEdge_LocYi: %d \n", k);
	return 0;
}

int Find_LowEdge(imageIP * TGTImage, ImgRectC *ChkBlock, int MinPixels, int MinLines, unsigned char MarkV)
{

	unsigned char TData;
	int i, j, k, PixelCount, LineCount;
	
	PixelCount=0;
	LineCount=0;
	k = ChkBlock->oyi+ ChkBlock->yr;
/*
	log_info("Find_LowEdge: ChkBlock->oyi = %d, oxj = %d, yr = %d, xc = %d \n",
			ChkBlock->oyi,
			ChkBlock->oxj,
			ChkBlock->yr,
			ChkBlock->xc);
*/
	for (i=k; i > ChkBlock->oyi; i--)		//from High to low
	{
		if (LineCount >= MinLines)
		{
			LineCount=i+LineCount;
//			log_info("Find_LowEdge: %d \n",LineCount);
			return LineCount;
		}
		if (PixelCount < MinPixels)
		{
			LineCount=0;
		}
		PixelCount=0;		
		for (j = ChkBlock->oxj; j < ChkBlock->oxj+ChkBlock->xc; j++) //from left to right
		{
			TData = TGTImage->data[i][j];
			if (TData == MarkV)
				PixelCount++;
			if (PixelCount >= MinPixels)
			{
				LineCount++;
				break;
			}				
		}			
	}
//	log_info("NoFind_LowEdge_LocYi: %d \n",ChkBlock->oyi);
	return 0;
}

//from left to right
int Find_LeftEdge(imageIP * TGTImage, ImgRectC *ChkBlock, int MinPixels, int MinLines, unsigned char MarkV)
{

	unsigned char TData;
	int i, j, k, PixelCount, LineCount;
	
	PixelCount=0;
	LineCount=0;
	k = ChkBlock->oxj+ ChkBlock->xc-1;
/*
	log_info("Find_LeftEdge: ChkBlock->oyi = %d, oxj = %d, yr = %d, xc = %d \n",
			ChkBlock->oyi,
			ChkBlock->oxj,
			ChkBlock->yr,
			ChkBlock->xc);
*/
	for (j = k; j > ChkBlock->oxj; j--) //from left to right
	{
		if (LineCount >= MinLines)
		{
			LineCount=j+LineCount;
//			log_info("Find_LeftEdge_LocX: %d \n",LineCount);
			return LineCount;
		}
//		log_info("------------LeftLineCount: %d \n",LineCount);
		if (PixelCount < MinPixels)
		{
			LineCount=0;
		}
		PixelCount=0;		
		for (i=ChkBlock->oyi; i < ChkBlock->oyi+ ChkBlock->yr; i++) 	//from low to high
		{
			TData = TGTImage->data[i][j];
			if (TData == MarkV)
				PixelCount++;
			if (PixelCount >= MinPixels)
			{
				LineCount++;
				break;
			}				
		}			
	}
//	log_info("NoFind_LeftEdge_LocX: %d \n",k);
	return 0;
}

//from right to left
int Find_RightEdge(imageIP * TGTImage, ImgRectC *ChkBlock, int MinPixels, int MinLines, unsigned char MarkV)
{

	unsigned char TData;
	int i, j, k, PixelCount, LineCount;
	
	PixelCount=0;
	LineCount=0;
/*
	log_info("Find_RightEdge: ChkBlock->oyi = %d, oxj = %d, yr = %d, xc = %d \n",
			ChkBlock->oyi,
			ChkBlock->oxj,
			ChkBlock->yr,
			ChkBlock->xc);
*/
	for (j = ChkBlock->oxj; j < ChkBlock->oxj + ChkBlock->xc; j++) //from left to right
	{
		if (LineCount >= MinLines)
		{
			LineCount=j-LineCount;
//			log_info("Find_RightEdge_LocX: %d \n",LineCount);
			return LineCount;
		}
//		log_info("----------Right_LineCount: %d \n",LineCount);
		if (PixelCount < MinPixels)
		{
			LineCount=0;
		}
		PixelCount=0;		
		for (i=ChkBlock->oyi; i< ChkBlock->oyi+ChkBlock->yr; i++) 	//from low to high
		{
			TData = TGTImage->data[i][j];
			if (TData == MarkV)
				PixelCount++;
			if (PixelCount >= MinPixels)
			{
				LineCount++;
				break;
			}				
		}			
	}
	k = ChkBlock->oxj+ ChkBlock->xc;
//	log_info("NoFind_RightEdge_LocX: %d \n",k);
	return 0;
}

/*
These four fuctions check first line with the continuous Match_PixelNum of black pixels

int CHK_LowestLine(imageIP   * TGTImage, int Match_PixelNum)
int CHK_LeftLine(imageIP   * TGTImage, int Match_PixelNum)
int CHK_HighestLine(imageIP   * TGTImage, int Match_PixelNum)
int CHK_RightLine(imageIP   * TGTImage, int Match_PixelNum)
*/
/* 202020628: change to have a starting line */
//the startline can be at center or edge
//so it can be used to check from center to edge or from edge to center

//return the lowest line
int CHK_LowestLine(imageIP * TGTImage, int StartLine, int Match_PixelNum, int MatchLines, unsigned char MarkV)
{

	int i,j, is,js, ie, je, mm;
	unsigned char TData;
	
	is=js=ie=je=0;
	int k=Match_PixelNum;
	int mi= MatchLines;
	mm=0;
//	log_info("\n\r ----CHKP_Pos->BillNum = %d, HL = %d, LL = %d, LL = %d, RL = %d, Diff = %d \n\r",
//	for (i=0; i<TGTImage->iRect->yr; i++)
	for (i=StartLine; i<TGTImage->iRect->yr; i++)
	{
//		log_info(" LowLine: js = %d, je = %d, is = %d, ie = %d, mm = %d \n\r",
//			js,je,is,ie,mm);
		if (ie!=is)
		{
			mm++;
//			log_info(" LowLine: js = %d, je = %d, is = %d, ie = %d, mm = %d \n\r",
//			js,je,is,ie,mm);
		}
		if (mm > mi)
		{
////			log_info(" LowRTN1: js = %d, je = %d, is = %d, ie = %d, mm = %d \n",
//			js,je,is,ie,mm);
//			log_info("-------- LowLine: is = %d \n\r", is);
			return is;
		}else if (js == je)
			{
				is = i;
				ie = i;
				js = 0;
				je = 0;
				mm = 0;
			}			
		for (j = 0; j < TGTImage->iRect->xc; j++)
			{
				TData = TGTImage->data[i][j];
				if (TData == MarkV)
				{
					if (js==0)
					{
						js=j;
						je=j;
					}else 
					{
						je=j;
					}
//				log_info(" LowBlack: js = %d, je = %d, is = %d, ie = %d, mm = %d \n",
//						js,je,is,ie,mm);
					if (je >= (js + k-1))	//at least 3 black pixel
					{
						ie=i+1;
						break;		//stop checking this raw
					}				
				}else
				{
					js = 0;
					je = 0;
				}
			}
	}
	log_dbg(" RTN2: js = %d, je = %d, is = %d, ie = %d, mm = %d \n",
			js,je,is,ie,mm);
//	return is;
	return 0;
}

//[i][j]=[y][x]
//i=TGTImage->iRect->yr-1
int CHK_HighestLine(imageIP * TGTImage, int StartLine, int Match_PixelNum, int MatchLines, unsigned char MarkV)
{

	int i,j, is,js, ie, je, mm;
	unsigned char TData;
	
	is=js=ie=je=0;
	int k=Match_PixelNum;
	int mi= MatchLines;;
	mm=0;
//	log_info("\n\r ----CHKP_Pos->BillNum = %d, HL = %d, LL = %d, LL = %d, RL = %d, Diff = %d \n\r",
//	for (i=TGTImage->iRect->yr-1; i >0; i--)
	for (i=StartLine; i >0; i--)
	{
//		log_info(" HighLine: js = %d, je = %d, is = %d, ie = %d, mm = %d \n\r",
//			js,je,is,ie,mm);
		if (ie!=is)
			mm++;
//		log_info("---mm = %d \n\r", mm);
		if (mm > mi)
		{
//			log_info(" HighRTN1: js = %d, je = %d, is = %d, ie = %d, mm = %d mi = %d \n",
//			js,je,is,ie,mm, mi);
//			log_info("---------- HighLine: is = %d \n\r", is);
			return is;
		}else if (js == je)
		{
			is = i;
			ie = i;
			js = 0;
			je = 0;
			mm = 0;
		}			
		for (j = 0; j < TGTImage->iRect->xc; j++)
			{
				TData = TGTImage->data[i][j];
				if (TData == MarkV)
				{
					if (js==0)
					{
						js=j;
						je=j;
					}else 
					{
						je=j;
					}
//					log_info(" HighBlack: js = %d, je = %d, is = %d, ie = %d, mm = %d \n",
//						js,je,is,ie,mm);
					if (je >= (js + k-1))	//at least 3 black pixel
					{
						ie=i+1;
						break;		//stop checking this raw
					} 
				}else
				{
					js = 0;
					je = 0;
				}
			}
	}
	log_dbg(" RTN2: js = %d, je = %d, is = %d, ie = %d, mm = %d \n\r",
		js,je,is,ie,mm);
//	return is;
	return 0;
}

//[i][j]=[y][x]
int CHK_LeftLine(imageIP * TGTImage, int StartLine, int Match_PixelNum, unsigned char MarkV)
{

	int i,j, is,js, ie, je;
	unsigned char TData;
	
	is=js=ie=je=0;
	int k=(int)Match_PixelNum;
	
//	log_info(" Check LeftLine \n\r");
//	for (j=0; j<TGTImage->iRect->xc; j++)
	for (j=StartLine; j<TGTImage->iRect->xc; j++)
		for (i = 0; i < TGTImage->iRect->yr; i++)
		{
			TData = TGTImage->data[i][j];
			if (TData == MarkV)
				if (is==0)
				{
					is=i;
					ie=i;
				}else 
					ie=i;
			else if (ie < (is+k))	//at least 3 black pixel
				is=ie=0;
			else if (is!=ie)
			{
//				log_info(" LeftLine: j = %d \n\r", j);
				return j;
			}
				
		}
	return 0;
}

//[i][j]=[y][x]
int CHK_RightLine(imageIP * TGTImage, int StartLine,int Match_PixelNum, unsigned char MarkV)
{
	int i,j, is,js, ie, je;
	unsigned char TData;
	
	is=js=ie=je=0;
	int k=(int)Match_PixelNum;
//	log_info(" Check RightLine \n\r");
	
//	for (j=TGTImage->iRect->xc-1; j >= 0; j--)
	for (j=StartLine; j >= 0; j--)
		for (i = 0; i < TGTImage->iRect->yr; i++)
		{
			TData = TGTImage->data[i][j];
			if (TData == MarkV)
				if (is==0)
				{
					is=i;
					ie=i;
				}else 
					ie=i;
			else if (ie< (is+k))	//at least 3 black pixel
				is=ie=0;
			else if (is!=ie)
				{
//					log_info(" RightLine: j = %d \n\r", j);
					return j;
				}
				
		}
	return 0;

}

/*
These four function find the starting line with the number of black pixel in the continue lines
Each line need to have the number(Black_PixelNum) of black pixel

int CHK_HighBlkLine(imageIP   * TGTImage, int Black_PixelNum, int Continue_Lines)
int CHK_LowBlkLine(imageIP   * TGTImage, int Black_PixelNum, int Continue_Lines)
int CHK_LeftBlkLine(imageIP   * TGTImage, int Black_PixelNum, int Continue_Lines)
int CHK_RightBlkLine(imageIP   * TGTImage, int Black_PixelNum, int Continue_Lines)
*/

/*
CHK_HighBlkLine and CHK_LowBlkLine:

this set lines can be used to find the center lines.
1. For Check point
2. When looking for the region of a row of chars
because a row of chars might not be alighed, it can be used to locate the most possible center line among them
and then, when checking each char, based on the possible height of each char, it can move the top and bottom boundary one by one
until reach boarder with all white of horz lines (Min. 3 lines)
Expand the search from center lines

Note: it could be better to break the region containing the chars to left region and right region because it could be skewed.
		then, the error can be minimized.


*/

//check the lowest white line from top
//if the total number of black pixel in one horizontal line is less than the Black_PixelNum, it is white line
int CHK_HighBlkLine(imageIP   * TGTImage, int Black_PixelNum, int Continue_Lines)
{
	int  i, j, is, k;
	unsigned char TData;
	
	is=0;
	
	for (i=TGTImage->iRect->yr-1; i >= 0; i--)
	{
		k= 0;
		for (j = 0; j < TGTImage->iRect->xc; j++)
		{
			TData = TGTImage->data[i][j];
			if (TData == (unsigned char)BLACK)
				k++;
				
		}
		if (k > Black_PixelNum)
			is++;
		else
			is=0;
		if (is>Continue_Lines)
			return (i+Continue_Lines);
	}
	return 0;
}

int CHK_LowBlkLine(imageIP   * TGTImage, int Black_PixelNum, int Continue_Lines)
{
	int i,j, is, k;
	unsigned char TData;
	
	is=0;
	
	for (i=0; i<TGTImage->iRect->yr; i++)
	{
		k= 0;
		for (j = 0; j < TGTImage->iRect->xc; j++)
		{
			TData = TGTImage->data[i][j];
			if (TData == (unsigned char)BLACK)
			{
				k++;
			}				
		}
		if (k > Black_PixelNum)
			is++;
		else
			is=0;
		if (is>Continue_Lines)
			return (i-Continue_Lines);
	}
	return 0;
}

int CHK_LeftBlkLine(imageIP   * TGTImage, int Black_PixelNum, int Continue_Lines)
{
	int i,j, js, k;
	unsigned char TData;
	
	js=0;
	
	for (j=0; j<TGTImage->iRect->xc; j++)
	{
		k= 0;
		for (i = 0; i< TGTImage->iRect->yr; i++)
		{
			TData = TGTImage->data[i][j];
			if (TData == (unsigned char)BLACK)
			{
				k++;
			}				
		}
		if (k > Black_PixelNum)
			js++;
		else
			js=0;
		if (js>Continue_Lines)
			return (j-Continue_Lines);
	}
	return 0;
}


int CHK_RightBlkLine(imageIP   * TGTImage, int Black_PixelNum, int Continue_Lines)
{
	int i,j, js, k;
	unsigned char TData;
	
	js=0;
	
	for (j=TGTImage->iRect->xc-1; j >= 0; j--)
	{
		k= 0;
		for (i = 0; i< TGTImage->iRect->yr; i++)
		{
			TData = TGTImage->data[i][j];
			if (TData == (unsigned char)BLACK)
			{
				k++;
			}				
		}
		if (k > Black_PixelNum)
			js++;
		else
			js=0;
		if (js>Continue_Lines)
			return (j+Continue_Lines);
	}
	return 0;
}

/*
int CHK_LongestLine(imageIP   * TGTImage, int start_Line, int end_Line)
{

}
*/
/*
struct imgBlock_Loc_Set {
  int 	posx; 			//Org_X, offset in whole image
  int 	posy; 			//Org_Y, offset in whole image
  int  	Block_Width;			//xc, block Width    
  int  	Block_Height;			//yr, block Height
};
*/

/*20200510: temp removed for merging to M4, because the compilation error occured. Coding imcompledted */

void VRT_Center_Ana(int *PixelCountArray, int *VRTCntrArray, int xc)	//xc, 228, 240
{
	int *GP2, *GP3, *GP4;
	int *PArray, *SrcArray;
	int cnt, Pixel_Count;
	
	GP4 =(int *)malloc(sizeof(int)*(xc>>2));
	GP2 =(int *)malloc(sizeof(int)*(xc>>1));
	GP3 =(int *)malloc(sizeof(int)*(xc/3));

	PArray=GP2;
	SrcArray=PixelCountArray;
	cnt=xc >> 1;	
	for (int j=0; j < cnt; j++)
	{	
		Pixel_Count=0;
		*PArray=0;
		for (int i = 0; i < 2; i++)
		{
			Pixel_Count=*SrcArray;
			SrcArray++;
		}
		*PArray=Pixel_Count;
		PArray++;
	}

	PArray=GP3;
	SrcArray=PixelCountArray;
	cnt=xc/3;	
	for (int j=0; j < cnt; j++)
	{	
		Pixel_Count=0;
		*PArray=0;
		for (int i = 0; i < 3; i++)
		{
			Pixel_Count=*SrcArray;
			SrcArray++;
		}
		*PArray=Pixel_Count;
		PArray++;
	}

	PArray=GP4;
	SrcArray=PixelCountArray;
	cnt=xc/4;	
	for (int j=0; j < cnt; j++)
	{	
		Pixel_Count=0;
		*PArray=0;
		for (int i = 0; i < 4; i++)
		{
			Pixel_Count=*SrcArray;
			SrcArray++;
		}
		*PArray=Pixel_Count;
		PArray++;
	}

	

}
void Count_HOZ_Blacks(imageIP   * TGTImage, int *PixelCountArray)
{

	int i, j;
	int yr = TGTImage->iRect->yr;
	int xc = TGTImage->iRect->xc;
	int Pixel_Count;
	int *PArray;

	PArray= PixelCountArray;
	
	for ( i=0; i < yr; i++)
	{	
		Pixel_Count=0;
		*PArray=0;
		for ( j = 0; j < xc; j++)
			if (TGTImage->data[i][j] == (unsigned char)BLACK)
				Pixel_Count++;
		*PArray=Pixel_Count;
	}
}

void Count_VRT_Blacks(imageIP   * TGTImage, int *PixelCountArray)
{
	int i, j;
	int yr = TGTImage->iRect->yr;
	int xc = TGTImage->iRect->xc;
	int Pixel_Count;
	int *PArray;

	PArray= PixelCountArray;
	
	for ( j = 0; j < xc; j++)
	{	
		Pixel_Count=0;
		*PArray=0;
			for ( i=0; i < yr; i++)
				if (TGTImage->data[i][j] == (unsigned char)BLACK)
					Pixel_Count++;
		*PArray=Pixel_Count;
	}
}
 

/*
int CHK_LongestLine(imageIP   * TGTImage, int start_Line, int end_Line)
{

}
*/
/*
struct imgBlock_Loc_Set {
  int 	posx; 			//Org_X, offset in whole image
  int 	posy; 			//Org_Y, offset in whole image
  int  	Block_Width;			//nc, block Width    
  int  	Block_Height;			//nr, block Height
};
*/

/*20200510: temp removed for merging to M4, because the compilation error occured. Coding imcompledted */
//get the number of horizontal white gap in an enclosure box within a big image
int Get_WHT_HOZ_Gap(imageIP   * TGTImage, imgBlock_Loc_Set * EnclImg_Box, int Minimun_Gap_Pixel, int Max_Gap_Num)
//void Get_WHT_HOZ_Gap(imageIP   * TGTImage, AlphaB_BlockSet   * OCRImg_Blk, int LeftLine, int RightLine, int Minimun_Gap_Width)
{
	int i,j, is,js, ie, je;
	unsigned char TData;
	int *Gap_Loc_Array;

	Gap_Loc_Array=(int *)malloc(sizeof(int)* 100);
	
	is=js=ie=je=0;
	int k=Minimun_Gap_Pixel;
	int Gap_Count=0;
	int Gap_Start= -1;
	int Gap_End=0;
	int Gap_Number=0;
	int LowLine=EnclImg_Box->posy;
	int HighLine=EnclImg_Box->posy+EnclImg_Box->Block_Height;
	int LeftLine=EnclImg_Box->posx;
	int RightLine=EnclImg_Box->posx+EnclImg_Box->Block_Width;
	
	for (i=LowLine; i < HighLine; i++)
	{	
		if (Gap_Start== -1)
		{	Gap_Start=i;
			Gap_End=0;
			Gap_Count=0;
		}
		TData = (unsigned char)WHITE;
		for (j = LeftLine; j< RightLine; j++)
			TData &= TGTImage->data[i][j];
		
		if (TData == (unsigned char)WHITE)
		{
			Gap_Count++;
			Gap_End=i;
		}else if (Gap_Count >=Minimun_Gap_Pixel)
			{
				if (Gap_Number >= Max_Gap_Num)
					return 0;
				*Gap_Loc_Array=Gap_Start;
				Gap_Loc_Array++;
				*Gap_Loc_Array=Gap_End;
				Gap_Loc_Array++;
				Gap_Number++;
				Gap_Start= -1;
			}else 
				Gap_Start= -1;
	}
	
	free(Gap_Loc_Array);
	return Gap_Number;	
}
//return the gap location starting from left 
//Minimun_Gap_Width, 4 for NTD at 200 dpi
//only check the pixels in between HighLine and LowLine   AlphaB_BlockSet   * OCRImg_Blk
void CHK_WHT_VRT_Gap(imageIP   * TGTImage, AlphaB_BlockSet   * OCRImg_Blk, int HighLine, int LowLine, int Minimun_Gap_Width)
{
	int i,j, is,js, ie, je;
	unsigned char TData;
	int *Gap_Loc_Array;
	
	is=js=ie=je=0;
	int k=Minimun_Gap_Width;
	int Gap_Count=0;
	int Gap_Start= -1;
	int Gap_End=0;
	int Gap_Number=0;
	Gap_Loc_Array=OCRImg_Blk->VRT_Edges;	

	for (j=0; j< TGTImage->iRect->xc; j++)
	{	
		if (Gap_Start== -1)
		{	Gap_Start=j;
			Gap_End=0;
			Gap_Count=0;
		}
		TData = (unsigned char)WHITE;
		for (i = LowLine; i < HighLine; i++)
			TData &= TGTImage->data[i][j];
		
		if (TData == (unsigned char)WHITE)
		{
			Gap_Count++;
			Gap_End=j;
		}else if (Gap_Count >=Minimun_Gap_Width)
			{
				if (Gap_Number >= OCRImg_Blk->Max_Char)
					return;
				*Gap_Loc_Array=Gap_Start;
				Gap_Loc_Array++;
				*Gap_Loc_Array=Gap_End;
				Gap_Loc_Array++;
				Gap_Number++;
				Gap_Start= -1;
			}else 
				Gap_Start= -1;
	}
	if (Gap_Count >=Minimun_Gap_Width)
	{
		*Gap_Loc_Array=Gap_Start;
		Gap_Loc_Array++;
		*Gap_Loc_Array=Gap_End;
		Gap_Number++;
	}
	OCRImg_Blk->VRT_Pairs = Gap_Number;
	return;
}

/*
struct AlphaB_BlockSet {
	int Char_Num;             	//Number of char in the block
	int *VRT_Edges;            	//vertical boundary pairs 
	int *HORZ_Edges;            //Horizontal boundary pairs
	unsigned char VRT_Pairs;   
	unsigned char HORZ_Pairs;   
	unsigned char  Max_Char;    //Max allowed char in the array, the allocated array size is two times of Max_Char +1
	unsigned char  Char_VRT;    //direction of char verical=1 or horizontal=0
};
*/
		

//Horz_BrdLoc_Array: based on the VRT_GapLoc_Array to get the boarder of each AlphaB
void Find_IndAlphaB_ImgBlk(imageIP   * TGTImage, AlphaB_BlockSet   * OCRImg_Blk, AlphaB_PSet *TAlphaB_PSet, int HighLine, int LowLine)
{
	int i,j, is,js, ie, je;
	unsigned char TData, BlackPixel_Count;
	int *Loc_Array, *Loc_Array2;
	
	int AlphaB_Right,  AlphaB_Left;
	int AlphaB_Right2,  AlphaB_Left2;
	int AlphaB_Top,  AlphaB_Bottom;
	int *HORZ_Edges;
	int *VRT_Edges;
	
	int Brd_Start;
	int Minimun_Gap_Width;
	int Brd_Count;
	int Mid_Line;
	unsigned char Char_Count;

	Minimun_Gap_Width=3;
/*
	TAlphaB_PSet->ABwidth=18;
	TAlphaB_PSet->ABheight=27;
	TAlphaB_PSet->ABwidth_Min=6;
	TAlphaB_PSet->ABheight_Min=9;
*/	
	Loc_Array=OCRImg_Blk->VRT_Edges;	
	Loc_Array++; //skip the first white area.

	int Max_Width=TAlphaB_PSet->ABwidth + (TAlphaB_PSet->ABwidth>>1); //set 1.5 as the max width
	
	HORZ_Edges= OCRImg_Blk->HORZ_Edges;
	VRT_Edges= OCRImg_Blk->VRT_Edges;

	int gap_num= OCRImg_Blk->VRT_Pairs;
	OCRImg_Blk->Char_Num =0;
	Char_Count=0;
	while (gap_num >= 1 )
	{
		
		AlphaB_Left=*Loc_Array;
		Loc_Array++;
		AlphaB_Right=*Loc_Array;
		Loc_Array++;
		
		AlphaB_Left++;		//place the on left boarder of the black image 
		AlphaB_Right--;		//place the on right boarder of the black image
		
		//check if the gap size is valid for one CHR
		if ((AlphaB_Right-AlphaB_Left) < TAlphaB_PSet->ABwidth_Min) //check against to mini-width. if the CHAR is too small, skip the white to combine next
		{
			//skip the samll one, it need to add to the next alpha,
			Loc_Array2=Loc_Array;
			
			AlphaB_Left2=*Loc_Array2;
			Loc_Array2++;
			AlphaB_Right2=*Loc_Array2;
			Loc_Array2++;
			//get the next one for comparing
			AlphaB_Left2++;		//place the on left boarder of the black image 
			AlphaB_Right2--; 	//place the on right boarder of the black image
			
			if ((AlphaB_Right2-AlphaB_Left) <= (TAlphaB_PSet->ABwidth+2)) //check against with normal width to be sure it is ok to combine 
			{
				AlphaB_Right=AlphaB_Right2;	//expand the width
				Loc_Array++;
				Loc_Array++;

				gap_num--;
			}
			else //it seems the combine is too big, then skip the current one to use the next one?
			{
				//still record the iRect
				*HORZ_Edges=LowLine;
				HORZ_Edges++;
				*HORZ_Edges=HighLine;
				HORZ_Edges++;
				
				
				*VRT_Edges=AlphaB_Left;
				VRT_Edges++;
				*VRT_Edges=AlphaB_Right;
				VRT_Edges++;
				
				AlphaB_Left=AlphaB_Left2;  	//use next one
				AlphaB_Right=AlphaB_Right2; 
				gap_num--;
			}	
		} // else if ((AlphaB_Right-AlphaB_Left) > (TAlphaB_PSet->ABwidth+2) ?? would it be too big too??

		//find the continuous 3 lines with two or more pixels as the boarder
		Mid_Line = (HighLine+LowLine)>>1;
		Brd_Count=0;
		Brd_Start=0;
		i=LowLine;
		while ((i <= Mid_Line) && (Brd_Count < 3))
		{	
			BlackPixel_Count=0;
			for (j = AlphaB_Left; j <= AlphaB_Right; j++)
			{	
				TData = TGTImage->data[i][j];
				if (TData == (unsigned char)BLACK)
					BlackPixel_Count++;
			}
			
			if (BlackPixel_Count >= 2)
			{	
				if (Brd_Count==0)
					Brd_Start=j;
				Brd_Count++;
			}else
				Brd_Count=0; //reset Brd_Count
			i++;
		}
		if (Brd_Count >= 3)
			AlphaB_Bottom=Brd_Start;
		else
			AlphaB_Bottom=LowLine;
		
		Brd_Count=0;
		Brd_Start=0;
		i=HighLine;
		TData=0xFF;
		while ((i >= Mid_Line) && (Brd_Count < 3))
		{	
			BlackPixel_Count=0;
			for (j = AlphaB_Left; j <= AlphaB_Right; j++)
			{	
				TData &= TGTImage->data[i][j];
				if (TData == (unsigned char)BLACK)
					BlackPixel_Count++;
			}
			
			if (BlackPixel_Count >= 2)
			{	
				if (Brd_Count==0)
					Brd_Start=j;
				Brd_Count++;
			}else
				Brd_Count=0; //reset Brd_Count
			i--;
		}
		
		if (Brd_Count >= 3)
			AlphaB_Top=Brd_Start;
		else
			AlphaB_Top=HighLine;

		
		*HORZ_Edges=AlphaB_Bottom;
		HORZ_Edges++;
		*HORZ_Edges=AlphaB_Top;
		HORZ_Edges++;
		
		
		*VRT_Edges=AlphaB_Left;
		VRT_Edges++;
		*VRT_Edges=AlphaB_Right;
		VRT_Edges++;

		gap_num--;
		Char_Count++;
	}

	OCRImg_Blk->Char_Num = Char_Count;
	
}




void freeAlphaB_BlockSet(AlphaB_BlockSet   * z)
{
	free(z->VRT_Edges);
	free(z->HORZ_Edges);
	free(z);
}


AlphaB_BlockSet   * Allocate_AlphaB_BlockSet (unsigned char Max_Char)
{

	int	i;
	
	AlphaB_BlockSet   * q;

// Allocate a structure for the SE //
		q = (AlphaB_BlockSet   *)malloc (sizeof( AlphaB_BlockSet));
		i=Max_Char<<1;
		if (q == 0)
		{
		  log_err ("Can't allocate AlphaB_BlockSet.\n");
		  return 0;
		}
		q->Max_Char = Max_Char;	
		
		q->VRT_Edges = (int *)malloc(sizeof(int)*i); 
		if ((q->VRT_Edges==0)) 
		{
			free(q->VRT_Edges );
			log_err ("Out of storage in VRT_Edges.\n");
			return 0;
		}
		q->HORZ_Edges=  (int *) malloc(sizeof(int)*i );
		if ((q->HORZ_Edges==0)) 
		{
			free(q->VRT_Edges );
			free(q );
			log_err ("Out of storage HORZ_Edges.\n");
			return 0;
		}
		q->Char_Num = 0;	
		q->VRT_Pairs = 0;	
		q->HORZ_Pairs = 0;	
		q->Char_VRT = 0;	
	return q;
}

