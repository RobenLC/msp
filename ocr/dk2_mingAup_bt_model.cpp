/**********************************************************
Name :
Date : 2019/12/2
By   : liaojf
Final:
     : 2019/10/23: initial svm code
     : 2019/10/28: read 冠字號
	 : 2019/11/6 : show input image
	 : 2019/11/21 : try HOG feature
	 : 2019/12/9  : output decide character in order
	 : 2019/12/17 : add euler number, judge a list of images
	              : write result to one report file
     : 2019/12/24 : add center_weight
	 : 2019/12/30 : add dilation
	 : 2020/1/8   : deal img117-253 , img106-116
	 : 2020/1/21  : add report out
	 : 2020/1/22  : add y-pos constraint, add tw_base groups
   : 2020/2/6   : modify code for ubuntu  , path strings are different
   : 2020/3/9   : test speed up
   : 2020/3/9   : test org275_1 ~ org402_1
   : 2020/3/10  : test org403_1 ~ org428_1
   : 2020/3/11  : fontzie=[16,32], vector =108, kernel
   : 2020/3/12  : correct fignum value
   : 2020/3/12  : add left_cor to separate "B" + "8"
   : 2020/3/13  : improve accuracy
   : 2020/3/16  : for scan_flow test
   : 2020/3/16  : scan_flow test for DK2
   : 2020/3/17  : for LEO image (dark)
   : 2020/3/18  : remove large width font, height <20
   : 2020/3/19  : use NTU liblinear, 6 section codes
   : 2020/3/23  :  rm dummy image, dst, drawing
   : 2020/3/23  : remove dummy images ,dst, drawing
   : 2020/3/24  : remove bw
   : 2020/3/25  : use adaptive_thresh(51,20)
   : 2020/3/26  : font height >18
   : 2020/3/27  : add ming-image section7, handle shortage out_fonts
   : 2020/3/30  : equalizeHist,meanStdDev,calcHist: threshold value
   : 2020/3/31  : add section_9(img595~ img769)
   : 2020/4/1   : add section_10(img770~ img879)
   : 2020/4/8   : cropping 冠字號 block + section_11~12
   : 2020/4/9   : search top-left point from left-half-image area, bounding_rect.height >10
   : 2020/4/10  : defective  冠字號 problem
   : 2020/4/13  : left-top y-pos limit <55+ merged two images
   : 2020/4/14  : add F+I+O constraint; adaptive+fixed mixed mode
   : 2020/4/15  : AI training behavior :step1 : collect error fonts
                : add compare golden table
   : 2020/4/22  : Collect one 冠字號 set under no golden table for comparison
   : 2020/4/23  : denoise_img -> dilate -> invbw_img
   : 2020/4/24  : choose ocr error font into lib_model
   : 2020/4/27  : change center_pix conditon for "D"
   : 2020/4/28  : rm cvtColor
   : 2020/5/8   : 整合前級OCR + 後級收集error font, 重新產生model
   : 2020/5/19  : 先做OCR+ 從全錯字中湊出2組字庫
   : 2020/5/20  : for DK2, OCR => two font sets; disable ouptut msg
   : 2020/5/28  : close inout streams
   : 2020/6/22  : mingAup model
**********************************************************/
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv/cv.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "svm.h"
#include "linear.h"
#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <math.h>
//#include <Windows.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

//#pragma comment(lib,"libsvm.lib")


using namespace cv;
using namespace std;
using namespace cv::ml;



#define ubuntu
//#define win7
#ifdef MAKELIB
void build_model_up( void )
#else
int main()
#endif
{
//char * main(){



   //====================================only build a new model
  //===================================
  //------original golden font
  //--- font lib
  Mat orgfont_img;
  char *libfont = "./mingAup_set%d/M%d.bmp";

  char fontname[100] = " ";


  int orghog_descr_size = 108;
  struct model* model_ntu;


  //-- org font
  vector<float> org_ders;
  vector<vector<float> > org_HOG;


  HOGDescriptor hog(
	  Size(16, 16), //winSize
	  Size(16, 16), //blocksize
	  Size(16, 16), //blockStride,
	  Size(8, 8), //cellSize,
	  9, //nbins,
	  1, //derivAper,
	  -1, //winSigma,
	  0, //histogramNormType,
	  0.2, //L2HysThresh,
	  0,//gammal correction,
	  64,//nlevels=64
	  1);

  //-----read original two font_libs
  for (int j = 1; j < 3; j++) {
	  //------- 2019/11/26
	  //----- 建立冠字號font feature SVM
	  for (int i = 1; i < 37; i++)
	  {
		  sprintf(fontname, libfont, j, i);
		  orgfont_img = imread(fontname, CV_LOAD_IMAGE_GRAYSCALE);

		 // cout << fontname << endl;

		  hog.compute(orgfont_img, org_ders);
		  //-- check
		  //
		  org_HOG.push_back(org_ders);

	  }
  }


  //----prepare features mat to build model
  //int orghog_descr_size = org_HOG[0].size();
  int vector_num = org_HOG.size();
  cout << "orghog_descr_size  : " << orghog_descr_size << endl;
  cout << "input vector num  : " << vector_num << endl;


#if 0
  struct parameter
  {
	  int solver_type;

	  /* these are for training only */
	  double eps;             /* stopping criteria */
	  double C;
	  int nr_weight;
	  int *weight_label;
	  double* weight;
	  double p;
  };

  for classification
	  s = 0, L2R_LR                L2 - regularized logistic regression(primal)
	  L2R_L2LOSS_SVC_DUAL   L2 - regularized L2 - loss support vector classification(dual)
	  s = 2, L2R_L2LOSS_SVC        L2 - regularized L2 - loss support vector classification(primal)
	  L2R_L1LOSS_SVC_DUAL   L2 - regularized L1 - loss support vector classification(dual)
	  s = 4, MCSVM_CS              support vector classification by Crammer and Singer
	  L1R_L2LOSS_SVC        L1 - regularized L2 - loss support vector classification
	  s6, L1R_LR                L1 - regularized logistic regression
	  L2R_LR_DUAL           L2 - regularized logistic regression(dual)
	  for regression
		  L2R_L2LOSS_SVR        L2 - regularized L2 - loss support vector regression(primal)
		  L2R_L2LOSS_SVR_DUAL   L2 - regularized L2 - loss support vector regression(dual)
		  L2R_L1LOSS_SVR_DUAL   L2 - regularized L1 - loss support vector regression(dual)

#endif

		  struct parameter param =
	  {
		  // L2R_L2LOSS_SVC,
		  // 0.01,
		  L2R_LR,
		  0.01,
		  // MCSVM_CS,
		  //0.1,
		  // L1R_LR,
		  //0.01,
		  1,
		  0,
		  NULL,
		  NULL,
		  0.1,
		  NULL
	  };



  //struct svm_problem prob;
  struct problem prob;

  prob.l = vector_num; //number of training examples

					   //--- 20200319 add "n" feature_vector elements
  prob.n = orghog_descr_size;

  double * tmp_y = new double[prob.l]; //--- total

  for (int z = 0; z < 2; z++) {
	  for (int s = 0; s < 36; s++)
	  {
		  tmp_y[z * 36 + s] = s + 1;

	  }
  }


  prob.y = tmp_y;


  //--- write HOG feature file

  feature_node** x = new feature_node *[prob.l];

  for (int n = 0; n < prob.l; n++) {

	  feature_node* x_space_t = new feature_node[orghog_descr_size + 1];//temp example feature vector

																		//---20200206  sample number


	  for (int z = 0; z < orghog_descr_size; z++) {

		  x_space_t[z].index = z + 1;
		  x_space_t[z].value = org_HOG[n][z];


	  }


	  //--- add vector_end element index=-1
	  x_space_t[orghog_descr_size].index = -1;
	  x_space_t[orghog_descr_size].value = 0;


	  //----- assign
	  x[n] = x_space_t;
  }


  prob.x = x;//Assign x to the struct field prob.x

			 //-- 20200319 add
  prob.bias = -1.0;

  //2019/11/29------ do svmtrain
  //--- 2019/12/04 ok
  // model_ntu = svm_train(&prob, &param);
  model_ntu = train(&prob, &param);

  //------- 2019/12/17 save svm model
  //int svm_save_model(const char *model_file_name,
  //			       const struct svm_model *model);

  char *model_file_name = "ming_upA_model_hv2.txt";

  //svm_save_model(model_file_name, model_ntu);
  save_model(model_file_name, model_ntu);


 // system("pause");

 // cvWaitKey(0);
  //destroyAllWindows();
 // return 0;

 }







