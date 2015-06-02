#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <time.h> 
#include "Extendible_Hash.h"

/*---------------------������-------------------*/
/*����һ��������������Ϊ�����ļ���������ĸ�Ŀ¼·��*/
int main(int argc,char ** argv){

	Exten_Model=MOST;								//��չģʽѡ��

	/*----------�ַ������������ڸ���������������·��----------*/
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

	/*----------�����ļ�----------*/
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

	/*----------���˵��----------*/
	printf("ҳ����Ϊ��%d��",PAGE_NUMBER);
	if(Exten_Model==MOST)
		printf("��λ��չ\n");
	else
		printf("��λ��չ\n");

	clock_t Start, End;								//���ڼ�¼ʱ��

	/*----------����ʼ----------*/
	IO=0;
	Initialize();									//��ʼ��
	printf("���ڶ�ȡ���ݽ�������...\n");
	Read_Tuple();									//���ļ��ж�ȡԪ�鲢��������
	printf("������ɣ���ϣͰ����%d��I/O������%d��\n",Bucket_Number,IO);
	Start=clock();
	IO=0;
	Search();										//��ѯ
	End=clock();
	printf("��ѯ��ϣ���ʱ��%f�룬I/O������%d��\n",(double)(End-Start)/CLOCKS_PER_SEC,IO);
	Del_All_Page();									//ɾ��ҳ�� �ͷſռ�
	if(Exten_Model==LEAST)
		remove(TEMP);								//ɾ����ʱ�ļ�
	return 0;

}

/*----------��Ҫ����----------*/

/*	ȫ�ֳ�ʼ��
	��������
	���ܣ�ҳ�桢Ͱ�Լ�ȫ�ֱ����ĳ�ʼ�������ڳ���һ��ʼ
		  ����ϣ������ʼ��Ϊ���ΪһͰ��Ϊ����״̬	
		  ��������ָ��ָ��0��ҳ�棬������ָ��ָ��1��ҳ�棬����0��ҳ�����ڴ��������1��ҳ�����ڶ��뻺����������ҳ�����ڴ��Ͱ*/
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

/*	��ȡ����
	��������
	���ܣ��������ļ���ÿ�ζ���ҳ���С������������������1��ҳ�棩
		  �ֱ���ȡ��ÿһ��Ԫ��󴫵ݵ���һ�������н���������Ľ���	*/
void Read_Tuple(){
	FILE* Read_Pointer=fopen(TBL,"rb");
	Tuple Temp_Tuple;
	long Read_Offset=0;
	int Lenth,i;
	while(!feof(Read_Pointer)){
		fseek(Read_Pointer,Read_Offset,SEEK_CUR);
		fread(Read_Buffer,PAGE_SIZE-1,1,Read_Pointer);
		IO++;
		Read_Buffer[PAGE_SIZE-1]='\0';				//ÿ�ζ����СΪҳ���С-1�����������ڶ����������ĩβ��ӡ�\0�������ڽ������ж��Ƿ񵽴�����ĩβ
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
		Read_Offset=-Lenth;							//���Ķ��ļ���ƫ�ƣ�������ȡ���Ĳ�������Ԫ�������һ�ε����ݶ�ȡ��
	}
	fclose(Read_Pointer);
}

/*	����������
	������Ԫ��
	���ܣ�����Ԫ���������ת��Ϊ����ֵ���ҵ�����ֵ��Ӧ��Ͱ��
		  Ȼ�󴫵�Ԫ����Ͱ��λ�ø���һ����������Ԫ�����Ͱ��	*/
void Make_Index(Tuple Temp_Tuple){

	int Needed_BucketID=Get_BucketID(Temp_Tuple);

	for(int i=0;i<PAGE_NUMBER-2;i++){				//ѭ�����ڲ���ָ��Ͱ�����ڴ�ҳ���е�λ��
		if(Bucket_Set[i].BucketID==Needed_BucketID){
			Put_Into_Bucket(Temp_Tuple,i);
			return;
		}
	}

	int Position,New_PageID=Get_Page(Position);	

	FILE* File_Pointer=fopen(BUCKET,"rb+");			//�����ѯʧ�ܣ���ʾָ��Ͱ�����ڴ��У���Ҫ����һ��IO��Ͱ�����ڴ�ҳ��
	Bucket_Load(Position,Needed_BucketID,New_PageID);
	fseek(File_Pointer,Needed_BucketID*PAGE_SIZE,SEEK_SET);
	fread(Bucket_Set[Position].Head,PAGE_SIZE,1,File_Pointer);
	IO++;
	Page_Set[New_PageID].Curr_Size=Get_Size_From_Bucket(Position);
	Put_Into_Bucket(Temp_Tuple,Position);
	fclose(File_Pointer);
}

/*	׼����Ͱ
	������Ԫ�顢ָ��Ͱ��λ��
	���ܣ��ж������������ڽ�Ԫ�����ָ��Ͱ֮ǰ������ȷ�������£�
		  ���Ͱ��ʣ��ռ䲻���Դ�Ÿ�����Ԫ��ʱ������Ͱ����
		  Ͱ����֮ǰ�ж��������Ƿ���Ҫ��չ����
		  Ͱ���Ѻ󣬸���Ԫ�鴫�ݻ���һ�������������ж���Ҫ�����Ͱ
		  ��������Ͱ������������չ����ֱ�ӰѸ���Ԫ�鴫�ݵ���һ�������������Ͱ��*/
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

/*	Ԫ����Ͱ
	������Ԫ�顢ָ��Ͱ��λ��
	���ܣ���Ԫ��ֱ�ӷ���ָ��Ͱ��	*/
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
	*(Bucket_Set[Position].Tail-1)+=Temp_Tuple.Lenth;							//���¸�Ͱ����ָ���λ��
	*(Bucket_Set[Position].Tail-2)+=1;											//����Ͱ��Ԫ������
	*(Bucket_Set[Position].Tail-3-Num*2)=Temp_Tuple.Lenth;						//Ͱĩβ�����ӦԪ��ĳ���
	*(Bucket_Set[Position].Tail-4-Num*2)=Offset;								//�Լ�ƫ����
	Page_Set[Bucket_Set[Position].PageID].Curr_Size-=(Temp_Tuple.Lenth+2*INT);	//����Ͱ��ʣ��ռ䣨��ҳ��ʣ��ռ䣩
	Page_Set[Bucket_Set[Position].PageID].Refered=true;							//��Ǹ�Ͱ�����ʹ�������ʱ��ҳ���û��㷨
}

/*	����ҳ��
	�����������ô���һ�����������ڼ�¼����ɹ����ҳ���Ӧ���ڴ��е�λ��
	���أ�ҳ���
	���ܣ�����һ��ҳ�棬����п���ҳ���򷵻ص�һ������ҳ�棨��ҳ���˳��
		  ���û�п���ҳ�棬����ʱ���û��㷨�����������ĸ�ҳ�沢��Ϊ����ҳ����
		  ����ǰ�����ҳ���ʼ��	*/
int Get_Page(int& Position){
	int i;
	for(i=2;i<PAGE_NUMBER;i++){											//�����Ƿ���ڿ���ҳ��
		if(Page_Set[i].Used==false){
			for(int j=0;j<PAGE_NUMBER-2;j++){
				if(Bucket_Set[j].BucketID==-1){
					Position=j;
					return i;
				}
			}
		}
	}
	while(Page_Set[Clock].Refered==true||Page_Set[Clock].Locked==true){	//ʹ��ʱ��ҳ���û��㷨�жϻ����ĸ�ҳ��
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
	fseek(File_Pointer,Page_Set[Clock].BucketID*PAGE_SIZE,SEEK_SET);	//��ԭҳ�������д���ļ����ʼ��ҳ�淵��ҳ���
	fwrite(Bucket_Set[Position].Head,PAGE_SIZE,1,File_Pointer);
	IO++;
	fclose(File_Pointer);
	Reset(Position);
	return Clock;
}

/*	����ҳ��
	������ҳ���Ӧ�ڴ��е�λ��
	���ܣ����ڽ�ָ��Ϊλ�õ�ҳ����г�ʼ������ҳ���Ӧ��Ͱ����ϢҲ������ʼ����	*/
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

/*	��ѯ����
	��������
	���ܣ���ȡ��ѯ�ļ�������ȡ��Ҫ��ѯ��������
	      ͨ�������õĹ�ϣ�����ҵ���������Ӧ��Ͱ�ţ���Ͱ�����ڴ棨���ڴ����Ѵ��ڸ�Ͱ������IO�������ҳ���Ͱ���ڴ�λ��
		  �����������Լ�Ͱλ�õ���һ�������н���ƥ����Ҳ��������	*/
void Search(){
	FILE* Search_Pointer=fopen(IN,"rb+");
	int time,Search_Key,Needed_BucketID,Needed_Offset,IndexID;
	fscanf(Search_Pointer,"%d",&time);

	while(time--){
		fscanf(Search_Pointer,"%d",&Search_Key);

		if(Exten_Model==MOST){						//��ø���������Ӧ������ֵ
			IndexID=Get_IndexID_Most(Search_Key,-1);
			Needed_Offset=Get_IndexID_Most(Search_Key,-1)/(PAGE_SIZE/INT);
		}
		else{
			IndexID=Get_IndexID_Least(Search_Key,-1);
			Needed_Offset=Get_IndexID_Least(Search_Key,-1)/(PAGE_SIZE/INT);
		}

		if(Index_Offset!=Needed_Offset){			//�ж��ڴ��е��������Ƿ������������Ҫ������ֵ�����������IO�������Ӧ��������
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

		for(int i=0;i<PAGE_NUMBER-2;i++){			//�ж�Ͱ�Ƿ����ڴ��У����������IO�������ڴ�
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

/*	ƥ�����
	����������������ӦͰ��λ��
	���ܣ���Ͱ�ڼ����������������ҵ�ƥ��ĺ�����뻺�������У����������ⲿ�ֲ�����ҳ�����Կ��˴����飩
		  ���������󣬶Ի��������Ԫ����п������򣬲�д���ļ���	*/
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

/*	�����������뺯��
	��������������顢Ԫ�������Լ����鳤��
	���ܣ�����������������������������򣬶�Ӧ��Ԫ������Ҳͬʱ����
		  �������ֻ�ǿ����������ں���������ʵ������ݹ麯����	*/
void Quick_Sort(int *a,Tuple* b,int Lenth){
    Quick_Sort_Recursion(a,b,0,Lenth-1);			//���ÿ�������ݹ麯��
}

/*	ɾ������ҳ��
	��������
	���ܣ��ͷ�����ҳ��ռ�õ��ڴ�ռ䣬���ڳ������ǰ	*/
void Del_All_Page(){
	for(int i=0;i<PAGE_NUMBER;i++)
		free(Page_Set[i].Pointer);
}

/*----------��������----------*/

/*	��ʼ��ҳ��
	������ҳ���
	���ܣ���ʼ��ָ��ҳ��Ļ�����Ϣ	*/
void InitPage(int PageID){
	Page_Set[PageID].BucketID=-1;
	Page_Set[PageID].Curr_Size=PAGE_SIZE;
	Page_Set[PageID].Pointer=malloc(PAGE_SIZE);
	Page_Set[PageID].Refered=false;
	Page_Set[PageID].Used=false;
	Page_Set[PageID].Locked=false;
}

/*	��ʼ��Ͱ
	������Ͱλ��
	���ܣ���ʼ��ָ��λ��Ͱ�Ļ�����Ϣ	*/
void Init_Bucket_Set(int Position){
	Bucket_Set[Position].BucketID=-1;
	Bucket_Set[Position].Head=NULL;
	Bucket_Set[Position].PageID=-1;
	Bucket_Set[Position].Position=Position;
	Bucket_Set[Position].Tail=NULL;
}

/*	Ͱ���غ���
	������ָ��Ͱλ�á�ָ��Ͱ���롢ҳ���
	���ܣ��ڸ�����ҳ����ָ����Ͱλ���м���һ�����������Ͱ������ʼ�������Ϣ	*/
void Bucket_Load(int Position,int BucketID,int PageID){
	Bucket_Set[Position].BucketID=BucketID;
	Bucket_Set[Position].PageID=PageID;
	Bucket_Set[Position].Head=(char*)Page_Set[PageID].Pointer;
	Bucket_Set[Position].Position=Position;
	Bucket_Set[Position].Tail=(int*)Page_Set[PageID].Pointer+PAGE_SIZE/INT-1;	//����Ͱĩ��ָ��
	*(Bucket_Set[Position].Tail)=1;												//Ͱ��ĩ���������ͷֱ�������¼��Ͱ�ֲ���ȡ�����ָ��Ϊλ���Լ�Ԫ������
	*(Bucket_Set[Position].Tail-1)=0;
	*(Bucket_Set[Position].Tail-2)=0;

	Page_Set[PageID].Curr_Size=PAGE_SIZE-3*INT;									//����Ͱʣ��ռ䣨ҳ��ʣ��ռ䣩
	Page_Set[PageID].BucketID=BucketID; 
	Page_Set[PageID].Used=true;
}

/*	����Ͱ�������ֵ
	������ָ��Ͱλ��
	���أ�����ֵ
	���ܣ���ָ����Ͱ�е�Ԫ����������ø�Ͱ��Ӧ������ֵ
		  ������Ͱ���ѹ����и�����������ֵ��Ӧ��Ͱ��	*/
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

/*	���Ͱ�ռ�
	������ָ��Ͱλ��
	���أ�Ͱʣ��ռ��С
	���ܣ�����ָ��Ͱ��ʣ���С�����ڴ��ļ��ж����Ͱ�����¼����ʣ���С	*/
int Get_Size_From_Bucket(int Position){
	int Offset=*(Bucket_Set[Position].Tail-1),Number=*(Bucket_Set[Position].Tail-2);
	return PAGE_SIZE-Offset-(Number*2+3)*INT;
}

/*	���Ͱ��
	������Ԫ��
	���أ�Ͱ��
	���ܣ�����������ó�ָ��Ԫ����������Ӧ�Ĵ��Ͱ��	*/
int Get_BucketID(Tuple Temp_Tuple){
	int Key,Needed_Offset;
	if(Exten_Model==MOST)							//�������ֵ
		Key=Get_IndexID_Most(Get_Search_Key(Temp_Tuple),-1);
	else
		Key=Get_IndexID_Least(Get_Search_Key(Temp_Tuple),-1);
	Needed_Offset=Key/(PAGE_SIZE/INT);
	if(Index_Offset!=Needed_Offset){				//�ж��ڴ����������Ƿ����ָ������ֵ
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

/*	���������
	������Ԫ��
	���أ�������
	���ܣ���ȡ����Ԫ���������	*/
int Get_Search_Key(Tuple Temp_Tuple){
	char Key[7];
	int i=0,j,key=0;
	while(Temp_Tuple.Info[i]!='|'){
		Key[i]=Temp_Tuple.Info[i];
		i++;
	}
	for(j=0;j<i;j++){
		key+=(Key[j]-48)*pow(10,double(i-1-j));		//��������ת����ʮ����
	}
	return key;
}

/*	��������
	������Ԫ��
	���أ������
	���ܣ��Ӹ���Ԫ������ȡ����������������ǰ������	*/
int Get_Part_Key(Tuple Temp_Tuple){
	char Part_Key[6];
	int i=0,j=0,key=0;
	while(Temp_Tuple.Info[i]!='|'){					//����������
		i++;
	}
	i++;
	while(Temp_Tuple.Info[i]!='|'){
		Part_Key[j]=Temp_Tuple.Info[i];
		j++;
		i++;		
	}
	for(i=0;i<j;i++){
		key+=(Part_Key[i]-48)*pow(10,double(j-1-i));//�������ת����ʮ����
	}
	return key;
}

/*	��������ݹ麯��
	��������������顢Ԫ�����顢����ͷ��������β��
	���ܣ��ÿ��������㷨������������������ͬ������Ԫ������ﵽ��Ԫ������Ĺ���	*/
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

/*-----��λ��չʹ��-----*/

/*	ҳ������������չ
	��������
	���ܣ����������չ����չ���Ӧλ�õ�Ͱ��һ��
		  ��Ϊ�Ǹ�λ��չ��������չ�������ֵ���Ӧ��ԭ������ֵ�Ĳ�ֵǡ������չǰ��������Ĵ�С
		  ����ֻ�����������ĩβ��Ϊ������㣬����һ��ԭ������������	*/
void Exten_Index_Itself_Most(){
	for(int i=0;i<pow(2,(double)Global_Depth);i++)
		Index[(int)(i+pow(2,(double)Global_Depth))]=Index[i];
	Global_Depth++;
}

/*	ҳ�����������չ
	��������
	���ܣ�����������չ�����ҳ���С������µ���������չʵ�֣���ҳ������չһ����������ҪIO����	*/
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

/*	Ͱ����
	��������Ҫ���ѵ�Ͱ��λ��
	���ܣ�����ָ����Ͱ������ԭͰ�е��������ݸ����µľֲ����������Ͱ�н��з��䣬���ú���Ӧ������ָ��	*/
void Bucket_Split_Most(int Position){
	int First_PageID,Second_PageID,First_SetNO,Second_SetNO;
	/*��������ҳ�棬���ڼ��ط��ѳ�����������Ͱ*/
	Page_Set[Bucket_Set[Position].PageID].Locked=true;					//��ԭͰռ�ݵ�ҳ���������ֹ��ʱ��ҳ���û��㷨����
	First_PageID=Get_Page(First_SetNO);
	Bucket_Load(First_SetNO,Bucket_Set[Position].BucketID,First_PageID);
	Page_Set[First_PageID].Locked=true;									//��һ������ɹ���ҳ��ͬ����������ֹ����
	Second_PageID=Get_Page(Second_SetNO);
	Bucket_Load(Second_SetNO,++Bucket_Number,Second_PageID);
	Bucket_Set[Position].BucketID=-2;									//��Ͱ�Ÿ�������һ����Ͱ�󣬳���ԭͰ��Ͱ�ţ�
	Page_Set[Bucket_Set[Position].PageID].Locked=false;					//����
	Page_Set[First_PageID].Locked=false;

	int Local_Depth=(*(Bucket_Set[Position].Tail))+1;					//���¾ֲ����
	(*(Bucket_Set[First_SetNO].Tail))=(*(Bucket_Set[Second_SetNO].Tail))=Local_Depth;

	int New_IndexID=Get_IndexID_From_Bucket(Position)+(int)pow(2,(double)Local_Depth-1),Exten_IndexID;
	int i,j;
	int FIRST=Get_IndexID_From_Bucket(Position);
	/*���ѹ��̣�����һ����Ͱӵ��ԭͰ��Ͱ�ţ���һ��Ͱ����Ͱ����������µ�Ͱ��
	  ������������չ�Ĺ����У�����Ͱ�Ž��и��ƣ�������Ͱ�ų��ֺ�Ҫ����Ӧ������ֵ���ģ�ָ����Ͱ
	  ������ѭ��ʵ���������*/
	for(i=0;i<(int)pow(2,double(Global_Depth-Local_Depth));i++){
		Exten_IndexID=New_IndexID+Exten_Decimal(i,Local_Depth);			//ȫ�������ֲ���Ȳ���ͬʱ��Ҫ�Դ�Ͱ��õ�����ֵ������չ���������λ�����п����ԣ�
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
	/*��ԭͰ������Ԫ�鰴���µ���ȷ��䵽������Ͱ��*/
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

	Reset(Position);													//������󣬳���ԭͰ������ʼ��ԭͰռ�ݵ�ҳ����Ϣ

}

/*	�������ֵ
	���������������ֲ���ȣ���ѡ��
	���أ���Ӧ����µ�����ֵ
	���ܣ���������ת��Ϊ��������ʽ�󣬸��ݸ�������Ƚ�ȡ��Ӧ���Ⱥ�ת����ʮ���������أ�������ֵ
		  ���û�����Ҫ�󣨴���-1������Ĭ��Ϊȫ�����
		  ��ȡʱ������չ��ʽ���н�ȡ���˴�Ϊ��λ��չ���ʴ����λ���λ��ȡ	*/
int Get_IndexID_Most(int Key,int _Depth){
	char Temp[24],Switch[24];
	int i=0,j,IndexID=0,Depth;
	if(Key!=1&&Key!=0){								//ת��Ϊ��������ʽ���ߵ�λ�洢���ˣ�
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
	if(_Depth!=-1)									//ѡ�����
		Depth=_Depth;
	else
		Depth=Global_Depth;
	if(i+1>Depth)
		i=Depth-1;
	for(j=0;j<=i;j++)								//���ߵ�λ˳�������ͬʱ��ȡ����
		Switch[j]=Temp[i-j];
	for(j=0;j<=i;j++){
		IndexID+=Switch[j]*(pow(2,double(i-j)));	//ת����ʮ����
	}
	return IndexID;
}

/*	��չ������������
	������Ŀ�겹�������ֲ����
	���أ���չ�������ֵ
	���ܣ��������е�����ֵ��ͨ��ȫ����Ƚ��д���ģ���Ͱ���Ѻ���Ͱ�ľֲ������ȻС��ȫ�����ʱ
		  ��ô��Ͱ��Ӧ������ֵ�Ͳ�ֹһ���磺
		  �ֲ����Ϊ2��ȫ�����Ϊ4������Ͱ�����ݵ�����Ϊ10����ô��Ӧ����ֵΪ0010��0110��1010��1110����������������������Ӧ���Ͱ
		  ������Ҫ���Ѻ�Ҫ����ȫ�������ֲ���ȵĲ�ֵ���������ȫ������µ�����ֵ����������ָ���Ͱ��
		  ������������Ǹ���������ܶ����ڵģ����ڼ����������������	*/
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


/*-----��λ��չʹ��-----*/	
/*�����ĸ������������ֿɶ�Ӧ��λ��չ���ĸ�������ԭ������ͬ������������ֻ������չ��ʽ�ϴӸ�λ��ɴӵ�λ��չ*/

void Exten_Index_Itself_Least(){
	for(int i=0;i<(int)pow(2,(double)Global_Depth);i++)
		Index[(int)pow(2,(double)(Global_Depth+1))-1-2*i]=Index[(int)pow(2,(double)(Global_Depth+1))-2-2*i]=Index[(int)pow(2,(double)Global_Depth)-1-i];
	Global_Depth++;
}

void Exten_Index_Page_Least(){
	/*�ú�������һ����ʱ�ļ�����������չ*/
	int i,j,k,Position,Temp_IndexID=Get_Page(Position);					//����һ��ҳ����Ϊ��ʱ�ļ��Ķ��뻺��
	int* Temp_Index=(int*)Page_Set[Temp_IndexID].Pointer;
	Page_Set[Temp_IndexID].Used=true;
	FILE* Index_Pointer=fopen(INDEX,"rb+");
	FILE* Temp_Pointer=fopen(TEMP,"rb+");

	fseek(Index_Pointer,Index_Offset*PAGE_SIZE,SEEK_SET);
	fwrite(Index,PAGE_SIZE,1,Index_Pointer);
	IO++;

	for(i=0;i<Index_Size;i++){											//���Ƚ������ļ���������ʱ�ļ���
		fseek(Index_Pointer,i*PAGE_SIZE,SEEK_SET);
		fread(Index,PAGE_SIZE,1,Index_Pointer);
		IO++;
		fseek(Temp_Pointer,i*PAGE_SIZE,SEEK_SET);
		fwrite(Index,PAGE_SIZE,1,Temp_Pointer);
		IO++;
	}


	for(i=0;i<Index_Size;i++){											//��Ӧ����ʱ�ļ�����ԭ������������չ����
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

