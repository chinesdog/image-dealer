#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include<iostream>
#include<algorithm>

using namespace std;
typedef  unsigned char  U8;
typedef  unsigned short U16;
typedef  unsigned int   U32;
#pragma  pack(1)
typedef struct
{
    //bmp header
    U16 bfType;        
    U32 bfSize;        
    U16 bfReserved1;
    U16 bfReserved2;
    U32 bfOffBits;    
    //DIB header
    U32 biSize;        
    U32 biWidth;       
    U32 biHeight;      
    U16 biPlanes;
    U16 biBitCount;    
    U32 biCompression;
    U32 biSizeImage;    
    U32 biXPelsPerMeter;
    U32 biYPelsPerMeter;
    U32 biClrUsed;
    U32 biClrImportant;
} BMP_HEADER;
#pragma  pack()
char fName0[256],fName[256];
U8 buffer[1200*1200*3];
U8 buffer_8[1200*1200];
U8 clrPal[256*4];
int dist_rgb[1000];//rgb之和分布 
int hist_gray[300];
BMP_HEADER  header;
void balance(int bitCount,int width,int height,int gap);
void graying(int width,int height);
int main()
{
    int fp;
	int flags=0;
   	printf("File name of the image to be processed: ");
    scanf("%s", fName0);
    strncat(fName,fName0,strlen(fName0)-4);
    strcat(fName,"_x.bmp");
    fp = open(fName0, O_RDONLY | O_BINARY);
    if(fp==-1) {
    	perror("open bmp file fail"); 
    	return 0;
	}   
	
    read(fp, &header, sizeof(BMP_HEADER));	
	int sizeImage;
	if(header.biBitCount==24) sizeImage=header.biWidth*header.biHeight*3;	
	else if(header.biBitCount==8) sizeImage=header.biWidth*header.biHeight;
    lseek(fp, header.bfOffBits, SEEK_SET);
    read(fp, buffer, sizeImage);	
    close(fp);
   	printf("图片位数%d",header.biBitCount);
    printf("选择对图像的操作为 \n (1)最简色彩平衡 (2)图片灰度化处理:");
    scanf("%d",&flags);
    if(flags==1) {
    	int gap;
		printf("选择的精度为\n");
		scanf("%d",&gap);
    	balance(header.biBitCount,header.biWidth,header.biHeight,gap);	
	}
  	else if(flags==2) graying(header.biWidth,header.biHeight);
    return 0;
}
void balance(int bitCount,int width,int height,int gap){
	int size=height*width;
	
	if(bitCount==8){
		U8 minVal=255,maxVal=0;
		for(int i=0;i<height;i++){
			for(int j=0;j<width;j++){
				hist_gray[buffer[i*width+j]]++;
			}
		}
		int sum=0,pix_head=0,pix_end=0;
		for(int i=255;i>=0;i--) {
			sum+=hist_gray[i];
			if(sum>=width*height*gap*0.01){
				pix_end=i;
				break;
			}
		}
		sum=0;
		for(int i=0;i<=255;i++){
			sum+=hist_gray[i];
			if(sum>=width*height*gap*0.01){
				pix_head=i;
				break;
			}
		}
		for(int i=0;i<height;i++){
			for(int j=0;j<width;j++){
				if(buffer[i*width+j]>pix_head&&buffer[i*width+j]<pix_end){
					minVal=min(minVal,buffer[i*width+j]);
					maxVal=max(maxVal,buffer[i*width+j]);
				}
			}
		}
		for(int i=0;i<height;i++){
			for(int j=0;j<width;j++){
				if(buffer[i*width+j]<=pix_head) buffer[i*width+j]=0;
				else if(buffer[i*width+j]>=pix_end) buffer[i*width+j]=255;
				else buffer[i*width+j]=255*(1.0*buffer[i*width+j]-minVal)/(maxVal-minVal);
				
			}
		}
	}
	else if(bitCount==24){
		U8 Max_val=0;
		for(int i=0;i<height;i++){
			for(int j=0;j<width;j++){
				int sum=buffer[i*width*3+j*3]+buffer[i*width*3+j*3+1]+buffer[i*width*3+j*3+2];
				dist_rgb[sum]++;
				Max_val=max(Max_val,buffer[i*width*3+j*3]);
				Max_val=max(Max_val,buffer[i*width*3+j*3+1]);
				Max_val=max(Max_val,buffer[i*width*3+j*3+2]);
			}
		}
		int sum=0;
		int Threshold=0;
		for(int i=765;i>=0;i--){
			sum+=dist_rgb[i];
			if(sum>width*height*gap*0.01){
				Threshold=i;
				break;
			}
		}
		int Avg_r=0,Avg_g=0,Avg_b=0;
		int cnt=0;
		for(int i=0;i<height;i++){
			for(int j=0;j<width;j++){
				int sum=buffer[i*width*3+j*3]+buffer[i*width*3+j*3+1]+buffer[i*width*3+j*3+2];
				if(sum>Threshold){
					Avg_r+=buffer[i*width*3+j*3+2];
					Avg_g+=buffer[i*width*3+j*3+1];
					Avg_b+=buffer[i*width*3+j*3];
					cnt++;
				}
			}
		}
		Avg_r/=cnt;
		Avg_g/=cnt;
		Avg_b/=cnt;
		for(int i=0;i<height;i++){
			for(int j=0;j<width;j++){
				int red=(1.0*buffer[i*width*3+j*3+2]*Max_val)/Avg_r;
				int green=(1.0*buffer[i*width*3+j*3+1]*Max_val)/Avg_g;
				int blue=(1.0*buffer[i*width*3+j*3]*Max_val)/Avg_b;
				if(red>255) red=255;
				if(green>255) green=255;
				if(blue>255) blue=255;
				buffer[i*width*3+j*3+2]=red;
				buffer[i*width*3+j*3+1]=green;
				buffer[i*width*3+j*3]=blue;
			}
		}
	}	
	

	int fp = open(fName, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY);
    if(fp==-1) {
    	cout<<"打开图片失败";
		return;	
	}
    
	int sizeImage;
	if(header.biBitCount==24) sizeImage=header.biWidth*header.biHeight*3;	
	else if(header.biBitCount==8) sizeImage=header.biWidth*header.biHeight;
	 
    header.bfOffBits = sizeof(BMP_HEADER) + ((header.biBitCount==8)?1024:0);
    header.biSizeImage = sizeImage;
    header.biClrUsed = (header.biBitCount==8)?256:0;
    
    write(fp, &header, sizeof(BMP_HEADER));
    
    if(header.biBitCount==8)
    {
        for(int i=0; i<256; i++)
        {
            clrPal[4*i] = clrPal[4*i+1] = clrPal[4*i+2] = i;
            clrPal[4*i+3] = 0;
        }
        write(fp, clrPal, 256*4);
    }

    write(fp, buffer, header.biSizeImage);
	printf("写入完成");
    close(fp);
}
void graying(int width,int height){
	for(int i=0;i<height;i++){
		for(int j=0;j<width;j++){
			U8 gray=0.11*buffer[i*width*3+j*3]+0.59*buffer[i*width*3+j*3+1]+
					0.3*buffer[i*width*3+j*3+2];
			buffer_8[i*width+j]=gray;
		}
	}
	
	int fp = open(fName, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY);
	if(fp==-1) {
		cout<<"打开图片失败";
		return; 
	}
	header.biWidth=width;
	header.biHeight=height; 
	
	header.biSizeImage=header.biWidth*header.biHeight;
	header.bfSize=header.biSizeImage+header.bfOffBits;
    header.biBitCount=8;  
    write(fp, &header, sizeof(BMP_HEADER));
    for(int i=0; i<256; i++)
    {
        clrPal[4*i] = clrPal[4*i+1] = clrPal[4*i+2] = i;
        clrPal[4*i+3] = 0;
    }
    write(fp, clrPal, 256*4);
    write(fp, buffer_8, header.biSizeImage);
	printf("写入完成");
    close(fp);	
}
//灰度化成功 
