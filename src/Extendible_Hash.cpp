#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <time.h> 
#include "Extendible_Hash.h"

/*---------------------主函数-------------------*/
/*带有一个启动参数，作为所有文件输入输出的根目录路径*/
int main(int argc,char ** argv){

	Exten_Model=MOST;								//扩展模式选择

	/*----------字符串操作，用于辅助启动函数设置路径----------*/
	strcpy(BUCKET,argv[1]);
	strcat(BUCKET,"\\hashbucket.out");
	strcpy(INDEX,argv[1]);
	strcat(INDEX,"\\hashindex.out");
	strcpy(IN,argv[1]);
	strcat(IN,"\\testinput.in");
	strcpy(OUT,argv[1]);
	strcat(OUT,"\\testoutput.out");
	strcpy(TBL,argv[1]);
	strcat(TBL,"\\lineitem.tbl");
	if(Exten_Model==LEAST){
		strcpy(TEMP,argv[1]);
		strcat(TEMP,"\\indextemp.out");
	}

	/*----------创建文件----------*/
	FILE* fp=fopen(BUCKET,"wb+");
	fclose(fp);
	fp=fopen(INDEX,"wb+");
	fclose(fp);
	fp=fopen(OUT,"wb+");
	fclose(fp);
	if(Exten_Model==LEAST){
		fp=fopen(TEMP,"wb+");
		fclose(fp);
	}

	/*----------输出说明----------*/
	printf("页面数为：%d，",PAGE_NUMBER);
	if(Exten_Model==MOST)
		printf("高位扩展\n");
	else
		printf("低位扩展\n");

	clock_t Start, End;								//用于记录时间

	/*----------程序开始----------*/
	IO=0;
	Initialize();									//初始化
	printf("正在读取数据建立索引...\n");
	Read_Tuple();									//从文件中读取元组并建立索引
	printf("建立完成！哈希桶数：%d，I/O次数：%d。\n",Bucket_Number,IO);
	Start=clock();
	IO=0;
	Search();										//查询
	End=clock();
	printf("查询完毕！耗时：%f秒，I/O次数：%d。\n",(double)(End-Start)/CLOCKS_PER_SEC,IO);
	Del_All_Page();									//删除页面 释放空间
	if(Exten_Model==LEAST)
		remove(TEMP);								//删除临时文件
	return 0;

}

/*----------主要函数----------*/

/*	全局初始化
	参数：无
	功能：页面、桶以及全局变量的初始化，用于程序一开始
		  将哈希索引初始化为深度为一桶数为二的状态	
		  将索引表指针指向0号页面，读数据指针指向1号页面，即将0号页面用于存放索引表，1号页面用于读入缓冲区，其余页面用于存放桶*/
void Initialize(){
	int i;
	for(i=0;i<PAGE_NUMBER;i++){
		InitPage(i);
		if(i<PAGE_NUMBER-2)
			Init_Bucket_Set(i);
	}

	Index=(int*)Page_Set[0].Pointer;
	Index[0]=0;
	Index[1]=1;
	Page_Set[0].Curr_Size-=2*INT;
	Page_Set[0].Used=true;
	Index_Size=1;
	Index_Offset=0;
	Global_Depth=1;

	Bucket_Load(0,0,2);
	Bucket_Load(1,1,3);
	Bucket_Number=1;

	Clock=2;

	Read_Buffer=(char*)Page_Set[1].Pointer;
	Page_Set[1].Used=true;
}

/*	读取数据
	参数：无
	功能：从数据文件中每次读入页面大小的数据量到缓冲区（1号页面）
		  分别提取出每一条元组后传递到下一个函数中进行索引表的建立	*/
void Read_Tuple(){
	FILE* Read_Pointer=fopen(TBL,"rb");
	Tuple Temp_Tuple;
	long Read_Offset=0;
	int Lenth,i;
	while(!feof(Read_Pointer)){
		fseek(Read_Pointer,Read_Offset,SEEK_CUR);
		fread(Read_Buffer,PAGE_SIZE-1,1,Read_Pointer);
		IO++;
		Read_Buffer[PAGE_SIZE-1]='\0';				//每次读入大小为页面大小-1的数据量，在读入的数据量末尾添加‘\0’，用于接下来判断是否到达数据末尾
		i=Lenth=0;
		while(Read_Buffer[i]!='\0'){
			if(Read_Buffer[i]=='\n'){
				Temp_Tuple.Lenth=Lenth+1;
				Make_Index(Temp_Tuple);
				Lenth=0;
				memset(Temp_Tuple.Info,0,sizeof(Temp_Tuple.Info));
			}
			else{
				Temp_Tuple.Info[Lenth]=Read_Buffer[i];
				Lenth++;
			}
			i++;
		}
		Read_Offset=-Lenth;							//更改读文件的偏移，将最后读取到的不完整的元组归入下一次的数据读取中
	}
	fclose(Read_Pointer);
}

/*	建立索引表
	参数：元组
	功能：根据元组的搜索键转换为索引值，找到索引值对应的桶号
		  然后传递元组与桶的位置给下一个函数，将元组放入桶内	*/
void Make_Index(Tuple Temp_Tuple){

	int Needed_BucketID=Get_BucketID(Temp_Tuple);

	for(int i=0;i<PAGE_NUMBER-2;i++){				//循环用于查找指定桶号在内存页面中的位置
		if(Bucket_Set[i].BucketID==Needed_BucketID){
			Put_Into_Bucket(Temp_Tuple,i);
			return;
		}
	}

	int Position,New_PageID=Get_Page(Position);	

	FILE* File_Pointer=fopen(BUCKET,"rb+");			//如果查询失败，表示指定桶不在内存中，需要进行一次IO将桶读入内存页面
	Bucket_Load(Position,Needed_BucketID,New_PageID);
	fseek(File_Pointer,Needed_BucketID*PAGE_SIZE,SEEK_SET);
	fread(Bucket_Set[Position].Head,PAGE_SIZE,1,File_Pointer);
	IO++;
	Page_Set[New_PageID].Curr_Size=Get_Size_From_Bucket(Position);
	Put_Into_Bucket(Temp_Tuple,Position);
	fclose(File_Pointer);
}

/*	准备入桶
	参数：元组、指定桶的位置
	功能：判定函数，用于在将元组放入指定桶之前的条件确定，如下：
		  如果桶内剩余空间不足以存放给定的元组时，进行桶分裂
		  桶分裂之前判断索引表是否需要扩展翻倍
		  桶分裂后，给定元组传递回上一个函数，重新判断需要放入的桶
		  若不发生桶分裂与索引扩展，则直接把给定元组传递到下一个函数将其放入桶内*/
void Put_Into_Bucket(Tuple Temp_Tuple,int Position){
	if(Temp_Tuple.Lenth+2*INT>Page_Set[Bucket_Set[Position].PageID].Curr_Size){
		if(*(Bucket_Set[Position].Tail)==Global_Depth){
			if((int)pow(2,(double)(Global_Depth+1))>(PAGE_SIZE/INT)){
				if(Exten_Model==MOST)
					Exten_Index_Page_Most();
				else
					Exten_Index_Page_Least();
			}
			else{
				if(Exten_Model==MOST)
					Exten_Index_Itself_Most();
				else
					Exten_Index_Itself_Least();
			}
		}
		if(Exten_Model==MOST)
			Bucket_Split_Most(Position);
		else
			Bucket_Split_Least(Position);
		Make_Index(Temp_Tuple);
		return;
	}
	Put_Into(Temp_Tuple,Position);
}

/*	元组入桶
	参数：元组、指定桶的位置
	功能：把元组直接放入指定桶内	*/
void Put_Into(Tuple Temp_Tuple,int _Position){
	int Position,Offset,Num,i;
	if(_Position!=-1)
		Position=_Position;
	else{
		for(int i=0;i<PAGE_NUMBER-2;i++)
			if(Bucket_Set[i].BucketID==Get_BucketID(Temp_Tuple)){
				Position=i;
				break;
			}
	}
	Offset=*(Bucket_Set[Position].Tail-1);
	Num=*(Bucket_Set[Position].Tail-2);
	for(i=0;i<Temp_Tuple.Lenth;i++)
		Bucket_Set[Position].Head[Offset+i]=Temp_Tuple.Info[i];
	*(Bucket_Set[Position].Tail-1)+=Temp_Tuple.Lenth;							//更新该桶空闲指针的位置
	*(Bucket_Set[Position].Tail-2)+=1;											//更新桶内元组数量
	*(Bucket_Set[Position].Tail-3-Num*2)=Temp_Tuple.Lenth;						//桶末尾加入对应元组的长度
	*(Bucket_Set[Position].Tail-4-Num*2)=Offset;								//以及偏移量
	Page_Set[Bucket_Set[Position].PageID].Curr_Size-=(Temp_Tuple.Lenth+2*INT);	//更新桶的剩余空间（即页面剩余空间）
	Page_Set[Bucket_Set[Position].PageID].Refered=true;							//标记该桶被访问过，用于时钟页面置换算法
}

/*	申请页面
	参数：按引用传递一个参数，用于记录申请成功后的页面对应在内存中的位置
	返回：页面号
	功能：申请一个页面，如果有空闲页面则返回第一个空闲页面（按页面号顺序）
		  如果没有空闲页面，则由时钟置换算法来决定换出哪个页面并作为空闲页返回
		  返回前会进行页面初始化	*/
int Get_Page(int& Position){
	int i;
	for(i=2;i<PAGE_NUMBER;i++){											//查找是否存在空闲页面
		if(Page_Set[i].Used==false){
			for(int j=0;j<PAGE_NUMBER-2;j++){
				if(Bucket_Set[j].BucketID==-1){
					Position=j;
					return i;
				}
			}
		}
	}
	while(Page_Set[Clock].Refered==true||Page_Set[Clock].Locked==true){	//使用时钟页面置换算法判断换出哪个页面
		if(Clock==PAGE_NUMBER-1)
			Clock=2;
		else
			Clock++;
		Page_Set[Clock].Refered=false;
	}
	for(i=0;i<PAGE_NUMBER-2;i++){
		if(Bucket_Set[i].PageID==Clock){
			Position=i;
			break;
		}
	}

	FILE* File_Pointer=fopen(BUCKET,"rb+");
	fseek(File_Pointer,Page_Set[Clock].BucketID*PAGE_SIZE,SEEK_SET);	//将原页面的内容写入文件后初始化页面返回页面号
	fwrite(Bucket_Set[Position].Head,PAGE_SIZE,1,File_Pointer);
	IO++;
	fclose(File_Pointer);
	Reset(Position);
	return Clock;
}

/*	重置页面
	参数：页面对应内存中的位置
	功能：用于将指定为位置的页面进行初始化（该页面对应的桶的信息也将被初始化）	*/
void Reset(int Position){
	int PageID=Bucket_Set[Position].PageID;
	free(Page_Set[PageID].Pointer);
	Bucket_Set[Position].BucketID=-1;
	Bucket_Set[Position].Head=NULL;
	Bucket_Set[Position].PageID=-1;
	Bucket_Set[Position].Position=-1;
	Bucket_Set[Position].Tail=NULL;

	Page_Set[PageID].BucketID=-1;
	Page_Set[PageID].Curr_Size=PAGE_SIZE;
	Page_Set[PageID].Pointer=malloc(PAGE_SIZE);
	Page_Set[PageID].Refered=false;
	Page_Set[PageID].Used=false;

}

/*	查询函数
	参数：无
	功能：读取查询文件，并提取需要查询的搜索键
	      通过建立好的哈希索引找到搜索键对应的桶号，将桶读入内存（若内存中已存在该桶则无需IO），并找出该桶的内存位置
		  传递搜索键以及桶位置到下一个函数中进行匹配查找并有序输出	*/
void Search(){
	FILE* Search_Pointer=fopen(IN,"rb+");
	int time,Search_Key,Needed_BucketID,Needed_Offset,IndexID;
	fscanf(Search_Pointer,"%d",&time);

	while(time--){
		fscanf(Search_Pointer,"%d",&Search_Key);

		if(Exten_Model==MOST){						//获得该搜索键对应的索引值
			IndexID=Get_IndexID_Most(Search_Key,-1);
			Needed_Offset=Get_IndexID_Most(Search_Key,-1)/(PAGE_SIZE/INT);
		}
		else{
			IndexID=Get_IndexID_Least(Search_Key,-1);
			Needed_Offset=Get_IndexID_Least(Search_Key,-1)/(PAGE_SIZE/INT);
		}

		if(Index_Offset!=Needed_Offset){			//判断内存中的索引表是否包含我们所需要的索引值，不在则进行IO，读入对应的索引表
			FILE* File_Pointer=fopen(INDEX,"rb+");
			fseek(File_Pointer,Index_Offset*PAGE_SIZE,SEEK_SET);
			fwrite(Index,PAGE_SIZE,1,File_Pointer);
			IO++;
			fseek(File_Pointer,Needed_Offset*PAGE_SIZE,SEEK_SET);
			fread(Index,PAGE_SIZE,1,File_Pointer);
			IO++;
			Index_Offset=Needed_Offset;
			fclose(File_Pointer);
		}

		Needed_BucketID=Index[IndexID-Index_Offset*(PAGE_SIZE/INT)];

		for(int i=0;i<PAGE_NUMBER-2;i++){			//判断桶是否在内存中，不在则进行IO，读入内存
			if(Bucket_Set[i].BucketID==Needed_BucketID){
				Match_And_Output(Search_Key,i);
				return;
			}
		}

		int Position,New_PageID=Get_Page(Position);
		FILE* File_Pointer=fopen(BUCKET,"rb+");
		Bucket_Load(Position,Needed_BucketID,New_PageID);
		fseek(File_Pointer,Needed_BucketID*PAGE_SIZE,SEEK_SET);
		fread(Bucket_Set[Position].Head,PAGE_SIZE,1,File_Pointer);
		IO++;
		Page_Set[New_PageID].Curr_Size=Get_Size_From_Bucket(Position);

		Match_And_Output(Search_Key,Position);

		fclose(File_Pointer);
	}
	fclose(Search_Pointer);
}

/*	匹配输出
	参数：搜索键、对应桶的位置
	功能：在桶内检索所有搜索键，找到匹配的后将其存入缓冲数组中（用于排序，这部分不限制页面所以开了大数组）
		  检索结束后，对缓冲数组的元组进行快速排序，并写入文件中	*/
void Match_And_Output(int Search_Key,int Position){
	Tuple Temp;
	int i,j,count=0,Part_Key_Set[100];
	Tuple Tuple_Set[100];
	FILE* File_Pointer=fopen(OUT,"rt+");
	for(i=0;i<*(Bucket_Set[Position].Tail-2);i++){
		Temp.Lenth=*(Bucket_Set[Position].Tail-3-i*2);
		for(j=0;j<Temp.Lenth;j++)
			Temp.Info[j]=Bucket_Set[Position].Head[*(Bucket_Set[Position].Tail-4-i*2)+j];
		if(Get_Search_Key(Temp)==Search_Key){
			Part_Key_Set[count]=Get_Part_Key(Temp);
			Tuple_Set[count]=Temp;
			count++;
		}
	}	
	Quick_Sort(Part_Key_Set,Tuple_Set,count+1);
	for(i=0;i<count+1;i++){
		fseek(File_Pointer,0,SEEK_END);
		fwrite(Tuple_Set[i].Info,Tuple_Set[i].Lenth,1,File_Pointer);
		IO++;
		fprintf(File_Pointer,"\r\n");
	}
	fprintf(File_Pointer,"-1\r\n");
	fclose(File_Pointer);
}

/*	快速排序输入函数
	参数：排序键数组、元组数组以及数组长度
	功能：根据搜索键数组的搜索键进行排序，对应的元组数组也同时排序
		  这个函数只是快速排序的入口函数，具体实现在其递归函数中	*/
void Quick_Sort(int *a,Tuple* b,int Lenth){
    Quick_Sort_Recursion(a,b,0,Lenth-1);			//调用快速排序递归函数
}

/*	删除所有页面
	参数：无
	功能：释放所有页面占用的内存空间，用于程序结束前	*/
void Del_All_Page(){
	for(int i=0;i<PAGE_NUMBER;i++)
		free(Page_Set[i].Pointer);
}

/*----------辅助函数----------*/

/*	初始化页面
	参数：页面号
	功能：初始化指定页面的基本信息	*/
void InitPage(int PageID){
	Page_Set[PageID].BucketID=-1;
	Page_Set[PageID].Curr_Size=PAGE_SIZE;
	Page_Set[PageID].Pointer=malloc(PAGE_SIZE);
	Page_Set[PageID].Refered=false;
	Page_Set[PageID].Used=false;
	Page_Set[PageID].Locked=false;
}

/*	初始化桶
	参数：桶位置
	功能：初始化指定位置桶的基本信息	*/
void Init_Bucket_Set(int Position){
	Bucket_Set[Position].BucketID=-1;
	Bucket_Set[Position].Head=NULL;
	Bucket_Set[Position].PageID=-1;
	Bucket_Set[Position].Position=Position;
	Bucket_Set[Position].Tail=NULL;
}

/*	桶加载函数
	参数：指定桶位置、指定桶号码、页面号
	功能：在给定的页面与指定的桶位置中加载一个给定号码的桶，并初始化相关信息	*/
void Bucket_Load(int Position,int BucketID,int PageID){
	Bucket_Set[Position].BucketID=BucketID;
	Bucket_Set[Position].PageID=PageID;
	Bucket_Set[Position].Head=(char*)Page_Set[PageID].Pointer;
	Bucket_Set[Position].Position=Position;
	Bucket_Set[Position].Tail=(int*)Page_Set[PageID].Pointer+PAGE_SIZE/INT-1;	//设置桶末端指针
	*(Bucket_Set[Position].Tail)=1;												//桶的末端三个整型分别用来记录该桶局部深度、空闲指针为位置以及元组数量
	*(Bucket_Set[Position].Tail-1)=0;
	*(Bucket_Set[Position].Tail-2)=0;

	Page_Set[PageID].Curr_Size=PAGE_SIZE-3*INT;									//设置桶剩余空间（页面剩余空间）
	Page_Set[PageID].BucketID=BucketID; 
	Page_Set[PageID].Used=true;
}

/*	根据桶获得索引值
	参数：指定桶位置
	返回：索引值
	功能：从指定的桶中的元组搜索键获得该桶对应的索引值
		  用于在桶分裂过程中辅助更改索引值对应的桶号	*/
int Get_IndexID_From_Bucket(int Position){
	int i=0;
	Tuple Temp;
	while(Bucket_Set[Position].Head[i]!='|'){
		Temp.Info[i]=Bucket_Set[Position].Head[i];
		i++;
	}
	Temp.Info[i]='|';
	if(Exten_Model==MOST)
		return Get_IndexID_Most(Get_Search_Key(Temp),*Bucket_Set[Position].Tail);
	else
		return Get_IndexID_Least(Get_Search_Key(Temp),*Bucket_Set[Position].Tail);
}

/*	获得桶空间
	参数：指定桶位置
	返回：桶剩余空间大小
	功能：返回指定桶的剩余大小，用于从文件中读入的桶中重新计算出剩余大小	*/
int Get_Size_From_Bucket(int Position){
	int Offset=*(Bucket_Set[Position].Tail-1),Number=*(Bucket_Set[Position].Tail-2);
	return PAGE_SIZE-Offset-(Number*2+3)*INT;
}

/*	获得桶号
	参数：元组
	返回：桶号
	功能：查找索引表得出指定元组搜索键对应的存放桶号	*/
int Get_BucketID(Tuple Temp_Tuple){
	int Key,Needed_Offset;
	if(Exten_Model==MOST)							//获得索引值
		Key=Get_IndexID_Most(Get_Search_Key(Temp_Tuple),-1);
	else
		Key=Get_IndexID_Least(Get_Search_Key(Temp_Tuple),-1);
	Needed_Offset=Key/(PAGE_SIZE/INT);
	if(Index_Offset!=Needed_Offset){				//判断内存中索引表是否包含指定索引值
		FILE* File_Pointer=fopen(INDEX,"rb+");
		fseek(File_Pointer,Index_Offset*PAGE_SIZE,SEEK_SET);
		fwrite(Index,PAGE_SIZE,1,File_Pointer);
		IO++;
		fseek(File_Pointer,Needed_Offset*PAGE_SIZE,SEEK_SET);
		fread(Index,PAGE_SIZE,1,File_Pointer);
		IO++;
		Index_Offset=Needed_Offset;
		fclose(File_Pointer);
	}
	return Index[Key-Index_Offset*(PAGE_SIZE/INT)];
}

/*	获得搜索键
	参数：元组
	返回：搜索键
	功能：提取给定元组的搜索键	*/
int Get_Search_Key(Tuple Temp_Tuple){
	char Key[7];
	int i=0,j,key=0;
	while(Temp_Tuple.Info[i]!='|'){
		Key[i]=Temp_Tuple.Info[i];
		i++;
	}
	for(j=0;j<i;j++){
		key+=(Key[j]-48)*pow(10,double(i-1-j));		//将搜索键转换成十进制
	}
	return key;
}

/*	获得排序键
	参数：元组
	返回：排序键
	功能：从给定元组中提取出排序键，用于输出前的排序	*/
int Get_Part_Key(Tuple Temp_Tuple){
	char Part_Key[6];
	int i=0,j=0,key=0;
	while(Temp_Tuple.Info[i]!='|'){					//跳过搜索键
		i++;
	}
	i++;
	while(Temp_Tuple.Info[i]!='|'){
		Part_Key[j]=Temp_Tuple.Info[i];
		j++;
		i++;		
	}
	for(i=0;i<j;i++){
		key+=(Part_Key[i]-48)*pow(10,double(j-1-i));//将排序键转换成十进制
	}
	return key;
}

/*	快速排序递归函数
	参数：排序键数组、元组数组、数组头部、数组尾部
	功能：用快速排序算法对排序键数组进行排序，同步更改元组数组达到对元组排序的功能	*/
void Quick_Sort_Recursion(int *a,Tuple* b,int Begin,int End){
    int i=Begin, j=End;
    int p=a[(i+j)/2];
    while(i<j)
    {
		for(;(i<End)&&(a[i]<p);i++);
		for(;(j>Begin)&&(a[j]>p);j--);
		if(i<=j)
		{
			int temp=a[i];
			a[i]=a[j]; 
			a[j]=temp;
			Tuple Temp=b[i];
			b[i]=b[j];
			b[j]=Temp;
			j--;
			i++;
		}
	}
    if(i<End)
		Quick_Sort_Recursion(a,b,i,End);
	if(j>Begin)
		Quick_Sort_Recursion(a,b,Begin,j);
}

/*-----高位扩展使用-----*/

/*	页面内索引表扩展
	参数：无
	功能：索引表的扩展，扩展后对应位置的桶号一样
		  因为是高位扩展，所以扩展后的索引值与对应的原来索引值的差值恰好是扩展前的索引表的大小
		  所以只需在索引表的末尾作为复制起点，拷贝一份原来的索引表即可	*/
void Exten_Index_Itself_Most(){
	for(int i=0;i<pow(2,(double)Global_Depth);i++)
		Index[(int)(i+pow(2,(double)Global_Depth))]=Index[i];
	Global_Depth++;
}

/*	页面间索引表扩展
	参数：无
	功能：在索引表扩展后大于页面大小的情况下的索引表扩展实现，与页面内扩展一样，这里需要IO处理	*/
void Exten_Index_Page_Most(){
	int i;
	FILE* File_Pointer=fopen(INDEX,"rb+");
	fseek(File_Pointer,Index_Offset*PAGE_SIZE,SEEK_SET);
	fwrite(Index,PAGE_SIZE,1,File_Pointer);
	IO++;
	for(i=0;i<Index_Size;i++){
		fseek(File_Pointer,i*PAGE_SIZE,SEEK_SET);
		fread(Index,PAGE_SIZE,1,File_Pointer);
		IO++;
		fseek(File_Pointer,(Index_Size+i)*PAGE_SIZE,SEEK_SET);
		fwrite(Index,PAGE_SIZE,1,File_Pointer);
		IO++;
	}

	Index_Size*=2;
	Index_Offset=i-1;

	Global_Depth++;
	fclose(File_Pointer);
}

/*	桶分裂
	参数：需要分裂的桶的位置
	功能：分裂指定的桶，并将原桶中的所有数据根据新的局部深度在两个桶中进行分配，设置好相应索引的指向	*/
void Bucket_Split_Most(int Position){
	int First_PageID,Second_PageID,First_SetNO,Second_SetNO;
	/*申请两个页面，用于加载分裂出来的两个新桶*/
	Page_Set[Bucket_Set[Position].PageID].Locked=true;					//给原桶占据的页面加锁，防止被时钟页面置换算法换出
	First_PageID=Get_Page(First_SetNO);
	Bucket_Load(First_SetNO,Bucket_Set[Position].BucketID,First_PageID);
	Page_Set[First_PageID].Locked=true;									//第一个申请成功的页面同样加锁，防止换出
	Second_PageID=Get_Page(Second_SetNO);
	Bucket_Load(Second_SetNO,++Bucket_Number,Second_PageID);
	Bucket_Set[Position].BucketID=-2;									//将桶号赋予其中一个新桶后，撤销原桶的桶号，
	Page_Set[Bucket_Set[Position].PageID].Locked=false;					//解锁
	Page_Set[First_PageID].Locked=false;

	int Local_Depth=(*(Bucket_Set[Position].Tail))+1;					//更新局部深度
	(*(Bucket_Set[First_SetNO].Tail))=(*(Bucket_Set[Second_SetNO].Tail))=Local_Depth;

	int New_IndexID=Get_IndexID_From_Bucket(Position)+(int)pow(2,(double)Local_Depth-1),Exten_IndexID;
	int i,j;
	int FIRST=Get_IndexID_From_Bucket(Position);
	/*分裂过程，其中一个新桶拥有原桶的桶号，另一个桶根据桶的数量获得新的桶号
	  而在索引表扩展的过程中，仅对桶号进行复制，所以新桶号出现后，要将对应的索引值更改，指向新桶
	  故以下循环实现这个功能*/
	for(i=0;i<(int)pow(2,double(Global_Depth-Local_Depth));i++){
		Exten_IndexID=New_IndexID+Exten_Decimal(i,Local_Depth);			//全局深度与局部深度不相同时需要对从桶获得的索引值进行扩展（即补充高位的所有可能性）
		int	Needed_Offset=Exten_IndexID/(PAGE_SIZE/INT);
		if(Index_Offset!=Needed_Offset){
			FILE* File_Pointer=fopen(INDEX,"rb+");
			fseek(File_Pointer,Index_Offset*PAGE_SIZE,SEEK_SET);
			fwrite(Index,PAGE_SIZE,1,File_Pointer);
			IO++;
			fseek(File_Pointer,Needed_Offset*PAGE_SIZE,SEEK_SET);
			fread(Index,PAGE_SIZE,1,File_Pointer);
			IO++;
			Index_Offset=Needed_Offset;
			fclose(File_Pointer);
		}
		Index[Exten_IndexID-Index_Offset*(PAGE_SIZE/INT)]=Bucket_Set[Second_SetNO].BucketID;
	}
	/*将原桶的所有元组按照新的深度分配到两个新桶中*/
	Tuple Temp;
	for(i=0;i<*(Bucket_Set[Position].Tail-2);i++){
		Temp.Lenth=*(Bucket_Set[Position].Tail-3-i*2);
		for(j=0;j<Temp.Lenth;j++)
			Temp.Info[j]=Bucket_Set[Position].Head[*(Bucket_Set[Position].Tail-4-i*2)+j];
		if(Get_IndexID_Most(Get_Search_Key(Temp),Local_Depth)==New_IndexID)
			Put_Into(Temp,Bucket_Set[Second_SetNO].Position);
		else
			Put_Into(Temp,Bucket_Set[First_SetNO].Position);
	}

	Reset(Position);													//分配完后，撤销原桶，即初始化原桶占据的页面信息

}

/*	获得索引值
	参数：搜索键、局部深度（可选）
	返回：对应深度下的索引值
	功能：将二进制转换为二进制形式后，根据给定的深度截取相应长度后转换成十进制数返回，即索引值
		  如果没有深度要求（传递-1），则默认为全局深度
		  截取时根据扩展形式进行截取，此处为高位扩展，故从最低位向高位截取	*/
int Get_IndexID_Most(int Key,int _Depth){
	char Temp[24],Switch[24];
	int i=0,j,IndexID=0,Depth;
	if(Key!=1&&Key!=0){								//转化为二进制形式（高低位存储反了）
		while(Key!=0&&Key!=1){
			Temp[i]=Key%2;
			Key=Key/2;
			i++;
		}
		Temp[i]=Key;
	}
	else{
		Temp[i]=Key;
	}
	if(_Depth!=-1)									//选择深度
		Depth=_Depth;
	else
		Depth=Global_Depth;
	if(i+1>Depth)
		i=Depth-1;
	for(j=0;j<=i;j++)								//将高低位顺序调换的同时截取数组
		Switch[j]=Temp[i-j];
	for(j=0;j<=i;j++){
		IndexID+=Switch[j]*(pow(2,double(i-j)));	//转换成十进制
	}
	return IndexID;
}

/*	扩展索引辅助函数
	参数：目标补充数、局部深度
	返回：扩展后的索引值
	功能：索引表中的索引值是通过全局深度进行储存的，当桶分裂后，新桶的局部深度仍然小于全局深度时
		  那么该桶对应的索引值就不止一个如：
		  局部深度为2，全局深度为4，假设桶内数据的索引为10，那么对应索引值为0010、0110、1010、1110四种情况，这四种情况都对应这个桶
		  所以需要分裂后要根据全局深度与局部深度的差值来获得所有全局深度下的索引值，并更改其指向的桶号
		  而这个函数则是辅助这个功能而存在的，用于计算出补充的所有情况	*/
int Exten_Decimal(int Target,int Local_Depth){
	char Temp[24];
	int i=0,j,Result=0;
	if(Target!=1&&Target!=0){
		while(Target!=0&&Target!=1){
			Temp[i]=Target%2;
			Target=Target/2;
			i++;
		}
		Temp[i]=Target;
	}
	else{
		Temp[i]=Target;
	}
	
	for(j=0;j<=i;j++)
		Result+=Temp[j]*(int)(pow(2,double(Local_Depth+j)));
	return Result;
}


/*-----低位扩展使用-----*/	
/*下面四个函数根据名字可对应高位扩展的四个函数，原理功能相同，不作累述，只是在扩展方式上从高位变成从低位扩展*/

void Exten_Index_Itself_Least(){
	for(int i=0;i<(int)pow(2,(double)Global_Depth);i++)
		Index[(int)pow(2,(double)(Global_Depth+1))-1-2*i]=Index[(int)pow(2,(double)(Global_Depth+1))-2-2*i]=Index[(int)pow(2,(double)Global_Depth)-1-i];
	Global_Depth++;
}

void Exten_Index_Page_Least(){
	/*该函数利用一个临时文件进行索引扩展*/
	int i,j,k,Position,Temp_IndexID=Get_Page(Position);					//申请一个页面作为临时文件的读入缓冲
	int* Temp_Index=(int*)Page_Set[Temp_IndexID].Pointer;
	Page_Set[Temp_IndexID].Used=true;
	FILE* Index_Pointer=fopen(INDEX,"rb+");
	FILE* Temp_Pointer=fopen(TEMP,"rb+");

	fseek(Index_Pointer,Index_Offset*PAGE_SIZE,SEEK_SET);
	fwrite(Index,PAGE_SIZE,1,Index_Pointer);
	IO++;

	for(i=0;i<Index_Size;i++){											//首先将索引文件拷贝到临时文件中
		fseek(Index_Pointer,i*PAGE_SIZE,SEEK_SET);
		fread(Index,PAGE_SIZE,1,Index_Pointer);
		IO++;
		fseek(Temp_Pointer,i*PAGE_SIZE,SEEK_SET);
		fwrite(Index,PAGE_SIZE,1,Temp_Pointer);
		IO++;
	}


	for(i=0;i<Index_Size;i++){											//对应着临时文件（即原索引）进行扩展翻倍
		fseek(Temp_Pointer,i*PAGE_SIZE,SEEK_SET);
		fread(Temp_Index,PAGE_SIZE,1,Temp_Pointer);
		IO++;
		k=0;

		for(j=0;j<(PAGE_SIZE/INT)/2;j++){
			Index[2*j]=Index[2*j+1]=Temp_Index[k];
			k++;
		}
		fseek(Index_Pointer,2*i*PAGE_SIZE,SEEK_SET);
		fwrite(Index,PAGE_SIZE,1,Index_Pointer);
		IO++;
		for(j=0;j<(PAGE_SIZE/INT)/2;j++){
			Index[2*j]=Index[2*j+1]=Temp_Index[k];
			k++;
		}
		fseek(Index_Pointer,(2*i+1)*PAGE_SIZE,SEEK_SET);
		fwrite(Index,PAGE_SIZE,1,Index_Pointer);
		IO++;
	}
	Index_Size*=2;
	Index_Offset=2*i-1;


	fclose(Index_Pointer);
	fclose(Temp_Pointer);
	int PageID=Bucket_Set[Position].PageID;
	free(Page_Set[PageID].Pointer);
	Page_Set[PageID].BucketID=-1;
	Page_Set[PageID].Curr_Size=PAGE_SIZE;
	Page_Set[PageID].Pointer=malloc(PAGE_SIZE);
	Page_Set[PageID].Refered=false;
	Page_Set[Temp_IndexID].Used=false;
	Global_Depth++;
}

void Bucket_Split_Least(int Position){
	int First_PageID,Second_PageID,First_SetNO,Second_SetNO;
	Page_Set[Bucket_Set[Position].PageID].Locked=true;
	First_PageID=Get_Page(First_SetNO);
	Bucket_Load(First_SetNO,Bucket_Set[Position].BucketID,First_PageID);
	Page_Set[First_PageID].Locked=true;
	Second_PageID=Get_Page(Second_SetNO);
	Bucket_Load(Second_SetNO,++Bucket_Number,Second_PageID);
	Bucket_Set[Position].BucketID=-2;
	Page_Set[Bucket_Set[Position].PageID].Locked=false;
	Page_Set[First_PageID].Locked=false;

	int Local_Depth=(*(Bucket_Set[Position].Tail))+1;

	(*(Bucket_Set[First_SetNO].Tail))=(*(Bucket_Set[Second_SetNO].Tail))=Local_Depth;

	int New_IndexID=2*Get_IndexID_From_Bucket(Position)+1,Exten_IndexID=New_IndexID;
	int i,j;
	int FIRST=2*Get_IndexID_From_Bucket(Position);
	for(i=0;i<Global_Depth-Local_Depth;i++)
		Exten_IndexID=2*Exten_IndexID;
	for(i=0;i<(int)pow(2,double(Global_Depth-Local_Depth));i++){
		Exten_IndexID=Exten_IndexID+i;
		int	Needed_Offset=Exten_IndexID/(PAGE_SIZE/INT);
		if(Index_Offset!=Needed_Offset){
			FILE* File_Pointer=fopen(INDEX,"rb+");
			fseek(File_Pointer,Index_Offset*PAGE_SIZE,SEEK_SET);
			fwrite(Index,PAGE_SIZE,1,File_Pointer);
			IO++;
			fseek(File_Pointer,Needed_Offset*PAGE_SIZE,SEEK_SET);
			fread(Index,PAGE_SIZE,1,File_Pointer);
			IO++;
			Index_Offset=Needed_Offset;
			fclose(File_Pointer);
		}

		Index[Exten_IndexID-Index_Offset*(PAGE_SIZE/INT)]=Bucket_Set[Second_SetNO].BucketID;
		Exten_IndexID=Exten_IndexID-i;
	}

	Tuple Temp;
	for(i=0;i<*(Bucket_Set[Position].Tail-2);i++){
		Temp.Lenth=*(Bucket_Set[Position].Tail-3-i*2);
		for(j=0;j<Temp.Lenth;j++)
			Temp.Info[j]=Bucket_Set[Position].Head[*(Bucket_Set[Position].Tail-4-i*2)+j];
		
		if(Get_IndexID_Least(Get_Search_Key(Temp),Local_Depth)==New_IndexID)
			Put_Into(Temp,Bucket_Set[Second_SetNO].Position);
		else
			Put_Into(Temp,Bucket_Set[First_SetNO].Position);				
	}

	Reset(Position);
}

int Get_IndexID_Least(int Key,int _Depth){
	char Temp[24];
	int i=0,IndexID=0,Depth,Lenth;
	if(Key!=1&&Key!=0){
		while(Key!=0&&Key!=1){
			Temp[i]=Key%2;
			Key=Key/2;
			i++;
		}
		Temp[i]=Key;
	}
	else{
		Temp[i]=Key;
	}
	if(_Depth!=-1)
		Depth=_Depth;
	else
		Depth=Global_Depth;
	Lenth=i+1;
	if(Depth<Lenth)
		Lenth=Depth;
	for(i=0;i<Lenth;i++)
		IndexID+=Temp[i]*(pow(2,double(Lenth-i-1)));
	if(Depth>Lenth)
		for(i=0;i<Depth-Lenth;i++)
			IndexID*=2;
	return IndexID;
}

