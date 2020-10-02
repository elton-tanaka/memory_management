#include <stdio.h>
#include <math.h>
//STL

/** Protótipo das funções de substituição de página **/
unsigned int aging();	//algoritmo do envelhecimento
unsigned int nru();		//algoritmo NRU (classes olhando R e M)
unsigned int clock();	//algoritmo do relógio
unsigned int lru();		//algoritmo LRU (mapa de bits na MMU)

void clock_tick_aging();	//tique do relógio - algoritmo do envelhecimento
void clock_tick_nru();		//tique do relógio - algoritmo NRU (classes olhando R e M)
void clock_tick_clock();	//tique do relógio - algoritmo do relógio
void clock_tick_lru();		//tique do relógio - algoritmo LRU (mapa de bits na MMU)

/// Configurações da Máquina
#define MEM_SIZE 10 	// 2^10 = 1KB

/// Configurações do Sistema Operacional
#define VIRT_MEM 12 	// 2^12 = 4KB
#define PAGE_SIZE 8 	// 8 bits ==> 256 bytes --> 8 quadros
											// --> 16 páginas
#define MAX_PROCESS 3 	// permitidos MAX_PROCESS
#define N_OP 5			// a cada 2 instruções um tick do relógio

unsigned int (*pageReplaceAlgorithm)() = &aging; // ponteiro para 
									// função de substituição de páginas

void (*clockTick)() = &clock_tick_aging; // ponteiro para tique do relógio
									// função de substituição de páginas

/** Estruturas da máquina **/

const unsigned int NUM_FRAMES = pow(2,(MEM_SIZE-PAGE_SIZE));

typedef struct _frame {
	unsigned int process_number;
	unsigned int page_number;
	unsigned char in_use;
	unsigned char age; 	// 8 bits para algoritmo de envelhecimento
}frame;

frame frames[NUM_FRAMES];

long long int getFirstFreeFrame(){
	for(unsigned int i=0;i<NUM_FRAMES;i++){
		if(frames[i].in_use==0) return i;
	}
	return -1;
}

void print_bits(unsigned char value){
	for(int i=0;i<8;i++){
		printf("%c",((value&0x80)>0)+'0');
		value <<=1;
	}
}

void showFrames(){
	printf("frame\tproc\tpage\tuse\tage\n");
	for(unsigned int i=0;i<NUM_FRAMES;i++){
		printf("%x\t%d\t%x\t%d\t",i,frames[i].process_number,frames[i].page_number,frames[i].in_use);
		print_bits(frames[i].age);
		printf("\n");
	}
}

/** Estruturas do sistema operacional **/

const unsigned int NUM_PAGES = pow(2,(VIRT_MEM-PAGE_SIZE));
const unsigned int MASK = (pow(2,VIRT_MEM)-1)-(pow(2,PAGE_SIZE)-1);

typedef struct _page_entry{
	unsigned char referenced;
	unsigned char modified;
	unsigned char present;
	unsigned int frame_number;
}page_entry;	

page_entry table_page[MAX_PROCESS][NUM_PAGES]; // aloca uma tabela de páginas para cada processo permitido no sistema.

int num_process=0;

void showPages(){
	printf("proc\tpage\tframe\tref\tmod\tpres\n");
	for(unsigned int i=0;i<MAX_PROCESS;i++){
		for(unsigned int j=0;j<NUM_PAGES;j++){
			printf("%d\t%x\t%x\t%d\t%d\t%d\n", i, j,
					table_page[i][j].frame_number,table_page[i][j].referenced,
					table_page[i][j].modified,table_page[i][j].present);
		}
	}
}

int _nops=0;

void _clockTick(){
	
	if(_nops==N_OP){
		(*clockTick)();
		_nops=0;
		printf("clock ticking\n");
	}
	else{
		_nops++;
	}
}

void assignPageFrame(int proc, int p, int f){
		table_page[proc][p].frame_number=f;
		table_page[proc][p].present=1;
		table_page[proc][p].referenced=0;
		table_page[proc][p].modified=0;
		frames[f].process_number=proc;
		frames[f].page_number=p;
		frames[f].in_use=1;
}

int memAlloc(int p, int size){

	_clockTick();
	
	if(size > pow(2,VIRT_MEM)) return -1; // não há memória virtual suficiente
	
	int n_pages = size/pow(2,PAGE_SIZE);
	printf("n_pages: %d\n",n_pages);
	
	// tentando reservar toda a memória necessária para o processo
	for(int i=0;i<n_pages;i++){
		long long int f = getFirstFreeFrame();
		if(f>=0){
			assignPageFrame(p,i,f); // processo p, pagina i, frame f
		}
		else{
			// 1) Seria interessante verificar se é possível realizar a
			//    substituição de páginas para alocar todo o processo?
			
			return -2; //algumas páginas não estão em memória
		}
	}
	return 1;
	
}

int accessMemory(int proc, int vaddr, char mode){
	
	_clockTick();
	
	if(vaddr >= pow(2,VIRT_MEM)) return -1; // fora do limite da memória

	int page = (vaddr & MASK)>>PAGE_SIZE;

	if(table_page[proc][page].present==1){
		int f = table_page[proc][page].frame_number;
		int fisAddr = (vaddr & ~MASK) + (f << PAGE_SIZE);
		printf("fisAddr: %x\n",fisAddr);
		table_page[proc][page].referenced=1;
		if(mode=='W'){
			table_page[proc][page].modified=1;
		}
	}
	else{
		printf("page fault\n");
		unsigned int s = (*pageReplaceAlgorithm)(); // descobre qual página substituir

		unsigned int proc_s = frames[s].process_number;
		unsigned int page_s = frames[s].page_number;
		printf("substituir quadro: %d | proc: %d | pagina: %x\n",
									s, proc_s, page_s);

		if(table_page[proc_s][page_s].modified==1)
			printf("gravando pagina no disco\n");
		
		table_page[proc_s][page_s].present=0;
		table_page[proc_s][page_s].referenced=0;
		table_page[proc_s][page_s].modified=0;
		
		assignPageFrame(proc,page,s); // processo p, pagina i, frame f
		printf("colocando pagina %x do processo %d no quadro %x\n", page,proc,s);
		int f = table_page[proc][page].frame_number;
		int fisAddr = (vaddr & ~MASK) + (f << PAGE_SIZE);
		printf("fisAddr: %x\n",fisAddr);
		table_page[proc][page].referenced=1;
		if(mode=='W'){
			table_page[proc][page].modified=1;
		}
	}
	return -2;
}

int main(){
	FILE *fp = fopen("process_operations01.txt","r");
	
	int p, value;
	char op;
	while(fscanf(fp, "%d %c %x", &p, &op, &value)!=-1){
		printf("processo: %d\n", p);
		printf("operacao: %c\n", op);
		printf("endereco: %x\n", value);
		if(op=='C'){
			printf("tentando alocar %d bytes para o processo %d\n",value, p);
			int res = memAlloc(p,value);
			if(res==-1){
				printf("Fatal error: required size greater than virtual memory\n");
				return -1;
			}else if(res==-2){
				printf("nem todas as paginas foram alocadas\n");
			}else
			{
				printf("todas as paginas alocadas com sucesso\n");
			}
		
		} else {
			int addr = accessMemory(p,value,op);
			if(addr==-1){
				printf("Fatal error: required addr outside virtual memory limit\n");
				return -1;
			}
		}
		showFrames();
		showPages();
		int a;
		scanf("%d",&a);
	}
	
	fclose(fp);
	return 0;
}

// algoritmo de aging

unsigned int aging(){
	int p_menor = 0; //posicao do menor
	unsigned int v_menor = frames[0].age + (table_page[frames[0].process_number][frames[0].page_number].referenced << 8); //valor do menor;
	for(unsigned int i=1;i<NUM_FRAMES;i++) {
		if((unsigned int)(frames[i].age + (table_page[frames[i].process_number][frames[i].page_number].referenced << 8))<v_menor) {
			p_menor=i;
			v_menor=frames[i].age;
		}
	}
	
	printf("Age do menor: ");
	printf("%d ", table_page[frames[p_menor].process_number][frames[p_menor].page_number].referenced);
	print_bits(frames[p_menor].age);
	printf("\n");
	
	frames[p_menor].age=0;
	
	return p_menor; // retorna quadro a ser substituido;
}

void clock_tick_aging(){
	for(unsigned int i=0;i<NUM_FRAMES;i++){
		unsigned char r = table_page[frames[i].process_number][frames[i].page_number].referenced;
		frames[i].age >>= 1; //deslocamento
		frames[i].age += (r << 7); // 8 bits no contador de idade; 
					// coloca o R na posição mais a esquerda do contador
		table_page[frames[i].process_number][frames[i].page_number].referenced=0;
	}
}

// algoritmo nru
unsigned int nru(){		// retorna o número do quadro que deve ser substituido.
	
	return 0;
	
}

void clock_tick_nru(){
	
	//percorrer a tabela de páginas (table_page[n_proc][n_pag]) colocando todos os bits referenced em 0.
	
}
