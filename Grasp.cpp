#include "Grasp.h"
#include "CommonDebug.h"

namespace std {

void getCurrentFolder(char *bufferOut, int sizeBufferOut)
{
    const int MAX_PATH_LENGTH = 200;
    char path[MAX_PATH_LENGTH];
    
    char *ret = getcwd(path, MAX_PATH_LENGTH);
    common_assert(NULL != ret);

    char bar[2] = {'/','\0'};
    strcat(path,bar);
    if((int)strlen(path) < sizeBufferOut)
    {
        sprintf(bufferOut, "%s", path);
    }
    else
    {
        bufferOut[0] = '\0';
    }
}

void dump_results_structured(int instance_size, char *instance_name, char *output, 
                             list<Sample> &samples1, list<Sample> &samples2, list<Sample> &samples3,
                                                     char* legenda1, char* legenda2, char *legenda3)
{
    char aux[300];
    char path_to_save[100];
    
	strcpy(aux,output);
	strcat(aux,".m");
	getCurrentFolder(path_to_save, sizeof(path_to_save));

	if(instance_size == SMALL_SIZE)
	{
	    strcat(path_to_save,(char*)"/autogenerated/small/");
    }
    else
    {
        strcat(path_to_save,(char*)"/autogenerated/large/");
    }

    strcat(path_to_save,aux);
    
    //printf("Salvaria no folder: %s\n", path_to_save);	
	FILE *fp = fopen(path_to_save,"w"); // Nome do arquivo: "output" + ".m"
	if(!fp)
	{
		printf("Nao foi possivel criar o arquivo %s. Abortando escrita...\n",path_to_save);
		return;
	}
	fprintf(fp,"v1 = [ ");
    for (Sample it : samples1 ) 
    {
        fprintf(fp,"%lf, %lf;\n",it.evaluation,it.time);
    }
	fprintf(fp," ];\n");
	
	fprintf(fp,"v2 = [ ");
    for (Sample it : samples2 ) 
    {
        fprintf(fp,"%lf, %lf;\n",it.evaluation,it.time);
    }
	fprintf(fp," ];\n");
	
	fprintf(fp,"v3 = [ ");
    for (Sample it : samples3 ) 
    {
        fprintf(fp,"%lf, %lf;\n",it.evaluation,it.time);
    }
	fprintf(fp," ];\n");

	fprintf(fp, "plot(v1(:,2),v1(:,1),'b',v2(:,2),v2(:,1),'r',v3(:,2),v3(:,1),'g');\nhold on\n");
	fprintf(fp, "title(\"Evolucao da Solucao na Instancia %s\");\n",instance_name);
	fprintf(fp, "xlabel(\"Tempo transcorrido [s]\");\n");
	fprintf(fp, "ylabel(\"Avaliacao da solucao\");\n");
	
	fprintf(fp, "h = legend (\"%s\", \"%s\", \"%s\");\n",legenda1, legenda2, legenda3);
	fprintf(fp, "set (h, \'fontsize\', 12)\n");
	
	fprintf(fp,"print('%s.png')",output);  // Nome do arquivo: "output" + ".png"
	fclose(fp);
}

int sol2vec(Solucao &s, int *v, int max_size)
{
	int pos = 0;
	for(int i = 0; i < s.quantidadeMaquinas(); i++)
	{

		for(list<Job>::iterator it = s.maquinas[i]->begin(); it != s.maquinas[i]->end(); it++)
		{
			if(pos > max_size-1)
			{
				printf("Acesso invalido em sol2vec\n");
				exit(0);
			}
			v[pos++] = it->id;
		}

		if(pos > max_size-1)
		{
			printf("Acesso invalido em sol2vec\n");
			exit(0);
		}
		if(i < s.quantidadeMaquinas()-1)
		{
			v[pos++] = -1;
		}

	}
	return pos;
}



Solucao vec2sol(int *v, int size, Solucao &s)
{
	int old_tar_qtd = s.quantidadeTarefas();
	for(int i = 0; i < s.quantidadeMaquinas(); i++)
	{
		s.maquinas[i]->clear();
	}
	int maq = 0;
	for(int i = 0; i < size; i++)
	{
		if(v[i] != -1)
		s.maquinas[maq]->push_back(v[i]);
		else
		maq++;
	}
	int new_tar_qtd = s.quantidadeTarefas();
	if (old_tar_qtd != new_tar_qtd)
	{
		printf("Erro na funcao vec2sol\n");
		exit(0);
	}

	return s;
}


Solucao_dummy dstep_relinking(Solucao_dummy *src, Solucao_dummy *dest,double alfa, double beta, 
                                                                                      Solucao &base)
{

	int cnt = 0;
	int swaps[MAX_JOBS][2];
	for(int i = 0; i < src->size; i++)
	{
		if(src->data[i] != dest->data[i])
		{
			int j = find(dest,src,src->data[i]);

			swaps[cnt][0] = i;
			swaps[cnt][1] = j;

			cnt++;
		}
	}
	if(cnt == 0)
	{
		Solucao_dummy igual = *src;
		return igual;
	}

	Solucao_dummy *solucoes[MAX_JOBS];
	for(int i = 0; i < cnt; i++)
	{
		solucoes[i] = geraVizinha(src,swaps[i][0],swaps[i][1]);
	}

	Solucao_dummy best = *(solucoes[0]);
	Solucao tempbest = base;
	tempbest = vec2sol(best.data,best.size,tempbest);
	otimiza_recursos(tempbest,alfa,beta);

	double best_ava = tempbest.avaliaSolucao(alfa,beta);
	for(int i = 0; i < cnt; i++)
	{
		Solucao temp_nodo = base;
		temp_nodo = vec2sol(solucoes[i]->data,solucoes[i]->size,temp_nodo);
		otimiza_recursos(temp_nodo,alfa,beta);
		double ava = temp_nodo.avaliaSolucao(alfa,beta);
		if(ava < best_ava)
		{
			best_ava = ava;
			best = *(solucoes[i]);
		}
	}

	for(int i = 0; i < cnt; i++)
	{
		free(solucoes[i]);
	}

	return best;
}


Solucao dpath_relinking(Solucao &a, Solucao &b, double alfa, double beta)
{

	Solucao_dummy *src = (Solucao_dummy *) malloc(sizeof(Solucao_dummy));
	Solucao_dummy *dest = (Solucao_dummy *) malloc(sizeof(Solucao_dummy));

	src->size = sol2vec(a,src->data,200);
	dest->size = sol2vec(b,dest->data,200);

	Solucao_dummy step = dstep_relinking(src, dest,alfa,beta,a);
	Solucao_dummy best = step;

	Solucao tempbest = a;
	tempbest = vec2sol(best.data,best.size,tempbest);
	otimiza_recursos(tempbest,alfa,beta);

	double best_ava = tempbest.avaliaSolucao(alfa,beta);

	while(!comparaSolucoes(&step,dest))
	{
		step = dstep_relinking(&step, dest,alfa,beta,a);
		Solucao tempstep;
		tempstep = tempbest;
		tempstep = vec2sol(step.data,step.size,tempstep);
		otimiza_recursos(tempstep,alfa,beta);

		double ava = tempstep.avaliaSolucao(alfa,beta);
		if(ava < best_ava)
		{
			best_ava = ava;
			best = step;
		}
	}
	free(src);
	free(dest);

	Solucao retorno = a;
	retorno = vec2sol(best.data,best.size,retorno);
	otimiza_recursos(retorno,alfa,beta);
	int ava_a = a.avaliaSolucao(alfa,beta);
	int ava_ret = retorno.avaliaSolucao(alfa,beta);
	int ava_b = b.avaliaSolucao(alfa,beta);
	if(ava_ret < ava_a && ava_ret < ava_b)
	{
	    return retorno;
	}
	else
	{
	    if(ava_a < ava_b)
	    {
	        return a;
	    }
	    else
	    {
	        return b;
	    }
	}
}

// Faz a ligação 2 a 2 de todas as soluções do pool.
Solucao path_relinking(list<Solucao> & pool, double alfa, double beta)
{
	if(pool.size() == 1)
	{
		return pool.front();
	}
	if(pool.size() == 0)
	{
		Solucao vazia;
		return vazia;
	}

	Solucao melhor = pool.front();
	int iteracoes = 0;
	for(list<Solucao>::iterator it1 = pool.begin(); it1 != pool.end(); it1++)
	{
		list<Solucao>::iterator it2 = it1;
		it2++;
		for(	; it1 != pool.end() && it2 != pool.end(); it2++)
		{
			iteracoes++;
			Solucao current = dpath_relinking(*it1, *it2, alfa, beta);
			if(current.avaliaSolucao(alfa,beta) < melhor.avaliaSolucao(alfa,beta))
			{
				melhor = current;
			}
		}
	}
	return melhor;
}



Solucao grasp_with_setings(Instancia *inst, PoliticaDeAlocacao politica, double alfa, double beta, 
                   int max_iter, double alfa_aleatoriedade, list<Sample> *samples, grasp_config set)
{
	Solucao construida, melhor_solucao;
	list<Solucao> pool;
	construida.bind(inst);
	melhor_solucao.bind(inst);

	double Zmin = ZINF;

    clock_t start_time = clock();
    clock_t new_timer = start_time;
    double delta;
    
	for(int cont = 0; cont < max_iter; cont++)
	{
		construcao_solucao(inst,construida,alfa,beta,politica,alfa_aleatoriedade);
        /* Neste trabalho, a única situação em que não se utiliza o SA é na busca por vizinhança */
        if(bvt == set)
        {
            construida = busca_local_iterada(construida,politica, alfa, beta);
        }
        else
        {
            construida = simulated_annealing(inst, 3.0, 80, 55, 0.96,alfa,beta,tipo_T);    
        }
		
		double avaliacao_corrente = construida.avaliaSolucao(alfa,beta);

        common_assert( avaliacao_corrente != 0 );
        common_assert( Zmin == ZINF );
		
        if(avaliacao_corrente < Zmin)
		{
			melhor_solucao = construida;
			Zmin = avaliacao_corrente;

            if( prsa == set )
            {
                alimenta_pool(pool, melhor_solucao, alfa, beta);    
            }
			
            new_timer = clock();
            delta = (new_timer - start_time) / (double)CLOCKS_PER_SEC;
            if (samples->size() != 0)
            {
                Sample previous = samples->back();
                storeSample(samples, previous.evaluation, delta);    
            }
            storeSample(samples, Zmin, delta); 
		}
	}
    
    if( prsa == set )
    {
    	Solucao s = path_relinking(pool, alfa, beta);
    	double ava = s.avaliaSolucao(alfa,beta);

    	if(ava < Zmin)
    	{
    	    Zmin = ava;
            new_timer = clock();
            delta = (new_timer - start_time) / (double)CLOCKS_PER_SEC;
            if (samples->size() != 0)
            {
                Sample previous = samples->back();
                storeSample(samples, previous.evaluation, delta);    
            }
            storeSample(samples, Zmin, delta); 
    	}
        melhor_solucao = s;
	}

    new_timer = clock();
    delta = (new_timer - start_time) / (double)CLOCKS_PER_SEC;
    storeSample(samples, Zmin, delta);
    
	return melhor_solucao;
}

}
