

typedef struct
{
    short   area_idx;		//number of image area fetched
    short   total_area;		//Total areat started
    short   image_idx;
    char    state;
//    char    state;
}   t_image_task_status;



int thr_glh(imageIP *NewImg, unsigned char Avg2Peak);
float u(float *p, int k);
float nu(float *p, int k, float ut, float vt);
float w(float *p, int k);

void APthreshold(imageIP *SrcImg, imageIP *DstImg, unsigned char ThresholdV, 
					unsigned char MinV, unsigned char MaxV); //THRESH_OTSU, THRESH_BINARY

