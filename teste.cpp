#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <cmath>
#include <vector>

using namespace std;


class aig{
    int *filho0_;
    int *filho1_;

    int *Arvore_index;//num_Invers // nível // nome sinal inicial // num consumidores + // num inversores do sinal // consumidores + inversores// proximo
    int *fanout_trees; //lista de arvores e fanout associados, cada arvore tem seu vetor arvore index
    //calculafanout-gera arvores desse vetor com algoritmo jody e gera vetor arvore index juntamente. 
    int max_fanout;// fanout maximo de entrada

    int maxvar_, ni_, no_, na_;
    set<int> entradas_;
    set<int> saidas_;
    int trata_linha_entrada_(string & linha);
    int trata_linha_saida_(string & linha);
    int trata_linha_and_(string & linha);
    int trata_linha_cabecalho_(string & linha, int &maxvar, int &ni, int &no, int &na);
    
    public:
    //aig(int maxvar);
    aig(string& filename);
    int print();
    int calculaAtraso();
    int *calculafaninout(int porta, set<int> &conp, set<int> &conn);
    void create_arv_inversores(int max_fanout);
    void genVerilog(string verilog_name);

};

void create_aiger(string &);
void create_aiger2(string &);
//int process_aiger_file(string& filename);
int *min_prof_arv(int fan_max,int ncp,int ncn);
int insere_arvore(int max_fanout,int index,int porta,int *nivs,set<int> &conp, set<int> &conn,vector<int> &arvore);

int main(int argc, char *argv[])
{
    if(argc < 5){
    cout << "Abrir com [nomedoarquivo.aag] [nodemoarquivodesaida.vh] [fanout] [0-sem biblioteca 1-skywater]"<< endl;
    return 0;
    }
    
    string name_aig = argv[1];
    string name_verilog = argv[2];
	int fan_max = atoi(argv[3]);
	int bib = atoi(argv[4]);
	
	
	int *saida;
    aig meu_aig(name_aig);
    meu_aig.calculaAtraso();
    meu_aig.create_arv_inversores(fan_max);
    meu_aig.genVerilog(name_verilog);

    return 0;
}


void create_aiger(string & filename)
{
    ofstream aiger_out(filename);
    aiger_out<<"aag 11 5 0 2 6"<<endl;
    aiger_out<<"2"<<endl;
    aiger_out<<"4"<<endl;
    aiger_out<<"6"<<endl;
    aiger_out<<"8"<<endl;
    aiger_out<<"10"<<endl;
    aiger_out<<"19"<<endl;
    aiger_out<<"23"<<endl;
    aiger_out<<"12 6 2"<<endl;
    aiger_out<<"14 8 6"<<endl;
    aiger_out<<"16 15 4"<<endl;
    aiger_out<<"18 17 13"<<endl;
    aiger_out<<"20 15 10"<<endl;
    aiger_out<<"22 21 17"<<endl;
    aiger_out.close();

}

void create_aiger2(string & filename)
{
    ofstream aiger_out(filename);
    aiger_out<<"aag 12 5 0 2 5"<<endl;
    aiger_out<<"2"<<endl;
    aiger_out<<"4"<<endl;
    aiger_out<<"6"<<endl;
    aiger_out<<"8"<<endl;
    aiger_out<<"10"<<endl;
    aiger_out<<"18"<<endl;
    aiger_out<<"21"<<endl;
    aiger_out<<"12 2 4"<<endl;
    aiger_out<<"14 7 8"<<endl;
    aiger_out<<"16 12 15"<<endl;
    aiger_out<<"18 12 16"<<endl;
    aiger_out<<"20 11 17"<<endl;
    aiger_out.close();

}


int aig::trata_linha_entrada_(string & linha)
{//coloca entrada na estrutura do AIG
    istringstream in(linha);
    int entrada;
    in >> entrada;
    cout <<"in="<< entrada << endl;
    entradas_.insert(entrada);
    return 0;
}

int aig::trata_linha_saida_(string & linha)
{//coloca saida na estrutura do AIG
    istringstream in(linha);
    int saida;
    in >> saida;
    cout <<"out="<< saida << endl;
    saidas_.insert(saida);
    return 0;
}

int aig::trata_linha_and_(string & linha)
{//coloca AND na estrutura do AIG
    istringstream in(linha);
    int saida, entrada0, entrada1;
    in >> saida;
    cout <<"conecta pino ="<< saida << endl;
    in >> entrada0;
    cout <<"conecta pino ="<< entrada0 << endl;
    in >> entrada1;
    cout <<"conecta pino ="<< entrada1 << endl;
    saida=saida/2;
    filho0_[saida]=entrada0;
    filho1_[saida]=entrada1;
    return 0;
}

int aig::trata_linha_cabecalho_(string & linha, int &maxvar, int &ni, int &no, int &na)
{
    istringstream in(linha);
    int l;
    string word;
    in >> word; cout<<"string discarded ="<<word<<endl;
    in >> maxvar; cout<<"number of variables is ="<<maxvar<<endl;
    in >> ni; cout<<"number of inputs is ="<<ni<<endl;  
    in >> l; cout<<"number of latches is ="<<l<<endl;  
    in >> no; cout<<"number of outputs is ="<<no<<endl;  
    in >> na; cout<<"number of ands is ="<<no<<endl;  
    maxvar_=maxvar; 
    ni_=ni; 
    no_=no; 
    na_=na;
    return 0;
}

aig::aig(string& filename)
{//copia do antigo int process_aiger_file(string& filename)
    int i=0;
    ifstream aiger_in(filename);
    char buffer[200];
    aiger_in.getline(buffer, 200, '\n');
    string cabecalho(buffer);
    int maxvar, ni, no, na;
    trata_linha_cabecalho_(cabecalho, maxvar, ni, no, na);
    filho0_=new int[maxvar+1];
    filho1_=new int[maxvar+1];
    Arvore_index= new int[5*(maxvar+2)];// cada and ou inversor terá 6 posiçoes no vetor a sua disposição.
    fanout_trees= new int [(maxvar+1)];// cada and ou inv terá 1 pos no vetor o indice ja diz o nó que tem fanout
    while (aiger_in.getline(buffer, 200, '\n'))
    {
        i++; //contador de linhas
        string s(buffer);
        if (i<=ni){
        cout<<"adiciona entrada"<<endl;
        trata_linha_entrada_(s);
        }
        else if (i<=(ni+no)){
        cout<<"adiciona saida"<<endl;
        trata_linha_saida_(s);
        }
        else if (i<=(ni+no+na)){
        cout<<"adiciona and"<<endl;
        trata_linha_and_(s);
        }
    }
}


int aig::print(){
     // printing set s1
    cout<<"Agora vou imprimir"<<endl;
    set<int>::iterator itr;
    for (itr = entradas_.begin(); itr != entradas_.end(); itr++) {
        cout <<"entrada =" << *itr << endl;
    }  
    //set<int>::iterator itr;
    for (itr = saidas_.begin(); itr != saidas_.end(); itr++) {
        cout <<"saida =" << *itr << endl;
    }   
    for (int i=ni_+1; i<(ni_+1+na_); i++)
    {
        cout << "AND   "<<i*2<<"="<<filho0_[i]<<"*"<<filho1_[i]<<endl;
    }
    return 0;
}


int aig::calculaAtraso(){
    cout<<maxvar_<<endl;
    int atrasos[maxvar_+1];
    int maxAtraso=0, af0, af1;
     // printing set s1
    cout<<"Agora vou calcular atrasos"<<endl;
    set<int>::iterator itr;
    //entradas tem atraso 0
    for (itr = entradas_.begin(); itr != entradas_.end(); itr++) {
      int x=*itr; x=x/2;
      atrasos[x]=0;
      cout <<"entrada "<<2*x<<" tem atraso=" << atrasos[x] << endl;
    }
    //calcula atraso das ands
    for (int i=ni_+1; i<(ni_+1+na_); i++)
    {
        cout << "AND   "<<i*2<<"="<<filho0_[i]<<"*"<<filho1_[i]<<endl;
        if((filho0_[i])% 2 != 0){
		af0=atrasos[(filho0_[i])/2]+1;cout<<"af0(+inverter)="<<af0<<endl;}
	else{
		af0=atrasos[(filho0_[i])/2];cout<<"af0="<<af0<<endl;}
	if(((filho1_[i]))% 2 != 0){
		af1=atrasos[(filho1_[i])/2]+1;cout<<"af1(+inverter)="<<af1<<endl;}
	else{
		af1=atrasos[(filho1_[i])/2];cout<<"af1="<<af1<<endl;}
		
	atrasos[i]=(af0 > af1 ? af0 : af1)+2;
	cout<<atrasos[i]<< "\ni=" << i <<endl;
    }
  
 
    //Qual saida tem o pior atraso?
    for (itr = saidas_.begin(); itr != saidas_.end(); itr++) {
  
     	if((*itr)% 2 != 0){ //se é inversora atraso anterior +1
    		af0=atrasos[((*(itr)-1)/2)]+1;
    		//cout<<"af0="<<af0<<endl;
    		}
    	else{// se é não inversora saída = atraso da saída
    	af0=atrasos[*(itr)/2];
    	//cout<<"af0="<<af0<<endl;
    	}	
    	
    	if(maxAtraso < af0){
    		maxAtraso=af0;}
    	
        cout <<"a saida " << *itr <<" tem o atraso ="<< maxAtraso <<endl;
    }
    cout << "atraso maximo é = " << maxAtraso << endl;
    return maxAtraso;

}
int *aig::calculafaninout(int porta, set<int> &conp, set<int> &conn){		//recebe uma porta e dois set conp(positivos) e conn(negativos), retorna fan_out[0] com consumidores positivos e fan_out[1] com negativos.
																			//adiciona consumidores ao set
	set<int>::iterator itr;
	int *fan_out = new int[2] ();
	
	for (int i=ni_+1; i<(ni_+1+na_); i++)
	{
		if(filho0_[i] == porta)
		{
			conp.insert(i*2);
			fan_out[0] ++;
			
		}
		else
			if(filho0_[i]-1 == porta)
			{
				conn.insert(i*2);
				fan_out[1] ++;
			}
		if(filho1_[i] == porta)
		{
			conp.insert(i*2);
			fan_out[0] ++;
		}
		else
			if(filho1_[i]-1 == porta)
				{
					conn.insert(i*2);
					fan_out[1] ++;
				}
		//cout << "contador "<<i<<" porta sendo vista "<<i*2<< " portas filhas "<<filho0_[i]<<" | "<<filho1_[i]<<endl;
	}
	
	for (itr = saidas_.begin(); itr != saidas_.end(); itr++)
	{
		if(*itr == porta)
			fan_out[0] += 1;
		if(*itr-1 == porta)
			fan_out[1] += 1;
	}
	
	//cout<<" Porta "<<porta <<" consumidores positivos "<<fan_out[0]<<" consumidores negativos " <<fan_out[1]<<endl;
	
	return fan_out;
}
void aig::create_arv_inversores(int max_fanout){
 	cout<<"Agora será criada a arvore de buffers inversores e sua estrutura."<<endl;
 	cout<<"Sendo implementada!"<<endl;
	int *num_niv;													
	int counter = 1;												// ajusta indice do vetor de arvores de acordo com o max_fanout e profundidade da arvore
	int *index = new int[ni_+na_] ();								// onde fica o index
	int *fan_out;													// vetor onde [0] contentem consumidores positivos e [1] negativos
	set<int> conp;													// set com consumidores positivos
	set<int> conn;													//set com consumidores negativos
	vector<int> arvore(100);
	
	for (int i=2; i<=(ni_+na_)*2; i=i+2)
	{
		fan_out = calculafaninout(i,conp,conn);										//calcula fanout da porta i e retornar resultado pra fan_out[0] e fan_out[1]
		
		if(fan_out[0] > max_fanout || fan_out[1] > 0)								//se precisar de arvore, calcula altura minima,adiciona ao vetor index onde comeca a arvore daquela porta 
		{
			num_niv = min_prof_arv(max_fanout,fan_out[0],fan_out[1]);
			index[i/2-1] = counter;
			counter = insere_arvore(max_fanout ,counter ,i ,num_niv ,conp ,conn ,arvore);
		}
			cout<<"portas consumidoras positivas "<<i<<endl;
			for (set<int>::iterator itr = conp.begin(); itr != conp.end(); itr++)
				cout<<*itr<<endl;
			cout<<"portas consumidoras negativas "<<i<<endl;
			for (set<int>::iterator itr = conn.begin(); itr != conn.end(); itr++)
				cout<<*itr<<endl;
			
			conp.clear();
			conn.clear();
	}

		//fan_out = calculafaninout(12,conp,conn);
		//num_niv = min_prof_arv(max_fanout,fan_out[0],fan_out[1]);
		//insere_arvore(max_fanout ,counter ,12 ,num_niv ,conp ,conn ,arvore);
		for (int value : arvore) {
        std::cout << value <<endl;
    }
	
	//for(int i =0;i<(ni_+na_);i++)
	//	cout <<"entrada index "<<i<<" porta "<< (i+1)*2 <<" posicao no vetor arvore "<<index[i]<<endl;
	return;
}

void aig:: genVerilog(string verilog_name){
    
    set<int> inversores;
    set<int>::iterator itr; 
    int outfinal, infinal;
    cout << "Agora vou criar o Verilog Netlist format" << endl;
    cout << "Criando arquivo Verilog: " << verilog_name << endl;

    ofstream verilogFile(verilog_name);

    if (!verilogFile.is_open()) {
        cout<< "Erro ao abrir o arquivo Verilog para escrita." << endl;
        return;
    }

    verilogFile << "module top_netlist(";
    for (itr = entradas_.begin(); itr != entradas_.end(); itr++) {
        verilogFile <<"in_"<<*itr  <<","; infinal=*itr;
    }
    for (itr = saidas_.begin(); itr != saidas_.end(); itr++) {
        if(itr++==saidas_.end()){
        	verilogFile <<"out_"<<*itr  <<",";}
       	else{ verilogFile <<"out_"<<*itr; outfinal=*itr;
       	}
        
    }
    verilogFile << ");"<< endl << "{" << endl;
    // Declare inputs and outputs
    
    for (itr = entradas_.begin(); itr != entradas_.end(); itr++) {
        verilogFile << " input " <<"in_" << *itr << ";" << endl;
    }

     //set<int>::iterator itr;
    for (itr = saidas_.begin(); itr != saidas_.end(); itr++) {
        verilogFile << " output "<<"out_" << *itr << ";" << endl;
    }
    
     verilogFile<< "}" << endl;
    // Declare wires for internal connections
    for (int i=ni_+1; i<(ni_+1+na_); i++)
    {
        verilogFile << "wire w"<< i*2 << ";" << endl;

        if(filho0_[i] % 2 != 0) 
        {
            //cria uma wire negado para f0, mas sem duplicar
            itr = inversores.find(filho0_[i]);
            if( itr == inversores.end() )
            {
                verilogFile << "wire w"<< filho0_[i] << ";" << endl;
                inversores.insert(filho0_[i]);
            }
        }

        if(filho1_[i] % 2 != 0) 
        {
            //cria uma wire negado para f0, mas sem duplicar
            itr = inversores.find(filho1_[i]);
            if( itr == inversores.end() )
            {
                verilogFile << "wire w"<< filho1_[i] << ";" << endl;
                inversores.insert(filho1_[i]);
            }
        }

 
    }
    for( itr = inversores.begin(); itr != inversores.end(); itr++)
    {
        verilogFile <<"not(" << "n" << *itr << ", n" << (*itr)-1 << ");" << endl;
    }

    // cria as ANDs
    for (int i=ni_+1; i<(ni_+1+na_); i++)
    {
        verilogFile << "and(n" << i*2 << ", n" << filho0_[i] << ", n" << filho1_[i] << ");" << endl;
   
    }
  
   
    cout << "Verilog Netlist gerado com sucesso: " << verilog_name << endl;

    // Close the Verilog file
    verilogFile.close();
}

int *min_prof_arv(int fan_max,int ncp,int ncn)	//algoritmo do paper do jody
{
	int pronto = 0;
	int i = 0;	//contador
	int ip = i;	//index candidato para positivos
	int in = i+1;	//index candidato pra negativo
	int vagas = int(pow(fan_max,i+1)); //numero de vagas por nivel
	
	if((vagas >= ncp) && (vagas - ncp)*fan_max >= ncn)
		pronto = 1;
	
	while(pronto == 0)
	{
		i++;
		vagas = int(pow(fan_max,i+1));
		if(!(i%2))
		{
			ip = i;
			in=i+1;
			if((vagas >= ncp) && (vagas - ncp)*fan_max >= ncn)
				pronto = 1;
		}
		else
		{
			in=i;
			ip=i+1;
			if((vagas >= ncn) && (vagas - ncn)*fan_max >= ncp)
				pronto = 1;
		}
	}
	int *index = new int[2];
	index[0] = ip;
	index[1] = in;
	return index;
}

int insere_arvore(int max_fanout,int index,int porta, int *nivs, set<int> &conp, set<int> &conn,vector<int> &arvore)		//utiliza niveis para pre alocar inversores, em seguida insere portas
{																															//nao organiza multiplos inversores por nivel
	int posi = index;												//	posicao onde sera colocado os identificadores
	cout<<"INDEX PORTA "<<porta<<" É "<<index;
	arvore[posi] = porta;
	posi += (max_fanout+1)*2;
	cout<<" NUMERO INDICE POSITIVO "<<nivs[0]<<" NIVEIS NEGATIVOS "<<nivs[1]<<endl; 
	int index_p = 1;
	int index_n= 1;
	for (int i = 1; i <= nivs[0]; i++)							//enquanto existirem niveis positivos, preenche o identificadores
	{
		arvore[posi] = porta;										//coloca identificador
		arvore[posi-1] = porta;										//coloca inversor no nivel anterior
		posi += (max_fanout+1)*2;
		
	}
	index_p = posi - (max_fanout+1);
	posi = index + (max_fanout+1);
	
	for (int i = 0; i <= nivs[1]; i ++)							//enquanto existirem niveis negativos, preenche o identificadores
	{
		arvore[posi] = porta+1;										//coloca identificador
		arvore[posi-1] = porta+1;									//coloca inversor no nivel anterior
		posi += (max_fanout+1)*2;
		
	}
	
	index_n = posi - (max_fanout+1);
	int nivel = index;
	int cont = 1;
	for (set<int>::iterator itr = conp.begin(); itr != conp.end(); itr++)	//insere consumidores positivos
	{
		
		if(cont == max_fanout)
		{
			if(arvore[nivel+cont] != 0)
			{
				nivel += (max_fanout+1)*2;
				cont = 1;
				arvore[nivel+cont] = *itr;
			}
			else
			{
				arvore[nivel+cont] = *itr;
				nivel += (max_fanout+1)*2;
				cont = 1;
				
			}
			
		}
		else
		{
			arvore[nivel+cont] = *itr;
		}
		cont ++;
	}
	
	nivel = index + (max_fanout+1);
	cont = 1;
		for (set<int>::iterator itr = conn.begin(); itr != conn.end(); itr++)	//insere consumidores positivos
	{
		
		if(cont == max_fanout)
		{
			if(arvore[nivel+cont] != 0)
			{
				nivel += (max_fanout+1)*2;
				cont = 1;
				arvore[nivel+cont] = *itr;
			}
			else
			{
				arvore[nivel+cont] = *itr;
				nivel += (max_fanout+1)*2;
				cont = 1;
				
			}
			
		}
		else
		{
			arvore[nivel+cont] = *itr;
		}
		cont ++;
	}
	
	return max(index_p,index_n);
}
