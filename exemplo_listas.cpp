#include <stdio.h>
#include <list>

using std::list;

typedef struct{
	char R;
	int nProc;
	int nPage;
}strRelogio;


int main(){
	
	list<strRelogio> lst;
	list<strRelogio>::iterator it;
	
	it = lst.begin();
	
	strRelogio a;
	a.R = 1;
	a.nProc=2;
	a.nPage=3;
	lst.push_back(a);

	a.R = 1;
	a.nProc=2;
	a.nPage=5;
	lst.push_back(a);
	
	a.R = 1;
	a.nProc=1;
	a.nPage=4;
	lst.push_back(a);
	
	if(it==prev(lst.begin())) it++;
	while(it->R==1){
		it->R = 0; 
		printf("%c %d %d\n", (it->R)+0x30, it->nProc, it->nPage);
		it++;
		if(it==lst.end())
			it = lst.begin();
	}
	printf("substituir: %c %d %d\n", (it->R)+0x30, it->nProc, it->nPage);
	
	return 0;
}
