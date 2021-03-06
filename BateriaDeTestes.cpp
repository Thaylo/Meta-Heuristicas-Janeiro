/*
 * BateriaDeTestes.cpp
 *
 *  Created on: Aug 11, 2014
 *      Author: thaylo
 */

#include "BateriaDeTestes.h"

#define CSVHEADER "Instance Name, Method, Final mean evaluation, Latest Time, Best Literature Evaluation, Best Evaluation Achieved, Reduction %\n"
const char csvHeader[] = CSVHEADER;

namespace std {

BateriaDeTestes::BateriaDeTestes() {}

static double evaluateAtTime(list<Sample> &ls, double time)
{
    double ret = 0;

    if ( time <= ls.front().time )
    {
        return ls.front().evaluation;
    }
    else if ( time >= ls.back().time )
    {
        return ls.back().evaluation;
    }
    else
    {
        for (Sample s : ls)
        {
            if ( time >= s.time )
            {
                ret = s.evaluation;
            }
            else
            {
                break;
            }
        }
    }
    return ret;
}

static list<Sample> sumSamples(list<Sample> &A, list<Sample> &B)
{
    list<Sample> auxiliar;
    list<Sample> final;

    double minTime = min(A.front().time, B.front().time);
    double maxTime = max(A.back().time, B.back().time);

    double currentTime = -1.0;
    double oldTime = -1.0;
    
    list<Sample>::iterator ita = A.begin();
    list<Sample>::iterator itb = B.begin();

    while(ita != A.end() || itb != B.end() )
    {
        if (ita == A.end() )
        {
            auxiliar.push_back(*itb);
            ++itb;
        }
        else if (itb == B.end())
        {
            auxiliar.push_back(*ita);
            ++ita;
        }
        else
        {
            if( ita->time < itb->time )    
            {
                auxiliar.push_back(*ita);
                ++ita;
            }
            else
            {
                auxiliar.push_back(*itb);
                ++itb;
            }
        }
    }

    list<Sample>::iterator itaux = auxiliar.begin();
    while( itaux->time < minTime )
    {
        ++itaux;
    }

    while(itaux != auxiliar.end() )
    {
        currentTime = itaux->time;
        if(itaux->time > maxTime)
        {
            break;
        }
        if ( currentTime != oldTime )
        {
            Sample s;
            s.time = itaux->time;
            s.evaluation = evaluateAtTime(A, currentTime) + evaluateAtTime(B, currentTime);
            final.push_back(s);
            oldTime = currentTime;
        }
        ++itaux;
    }
    return final;
}

static void multiplyByConstant(list<Sample> &ls, double constant)
{
    for(list<Sample>::iterator it = ls.begin(); it != ls.end(); ++it)
    {
        it->evaluation *= constant;
    }
}

void computeMeans(Summary *generalSummary);

static void writeDoubleOnCSV(FILE *fp, void *value)
{
    fprintf(fp, "%lf, ", *(double*)value);
}

static void writeIntOnCSV(FILE *fp, void *value)
{
    fprintf(fp, "%d, ", *(int*)value);
}

static void createCsvTable(AsyncChannel *async)
{
    async->write((void*) CSVHEADER, NULL);
}

void replace(char *str, char old, char newc)
{
    int i = 0;
    while( str[i] != '\0')
    {
        if(str[i] == old)
        {
            //cout << "Troquei " << old << " por " << newc << "\n"; 
            str[i] = newc;
        }
        i++;
    }
}

static void writeCsvLine(AsyncChannel *async, char *instanceName, double finalMean, double finalTime, 
                                                                    int bestLiteratureEvaluation,
                                                                    int bestEvaluationAchieved,
                                                                    char *method)
{
    char buffer[1024] = "";
    char bufferTmp[30];

    
    replace(instanceName, ';', '\'');
    strcat(buffer, instanceName);
    strcat(buffer, ",");

    replace(method, ';', '\'');
    strcat(buffer, method);
    strcat(buffer, ",");

    sprintf(bufferTmp, "%lf,", finalMean);
    strcat(buffer, bufferTmp);

    sprintf(bufferTmp, "%lf,", finalTime);
    strcat(buffer, bufferTmp);

    sprintf(bufferTmp, "%d,", bestLiteratureEvaluation);
    strcat(buffer, bufferTmp);

    sprintf(bufferTmp, "%d,", bestEvaluationAchieved);
    strcat(buffer, bufferTmp);

    if( bestEvaluationAchieved != -1 )
    {
        double percentualReduction = 
                     100.0*(bestLiteratureEvaluation - bestEvaluationAchieved)/bestLiteratureEvaluation;

        sprintf(bufferTmp, "%lf\n", percentualReduction);
        strcat(buffer, bufferTmp);
    }
    else
    {
        sprintf(bufferTmp, "-\n");
        strcat(buffer, bufferTmp);
    }  
    
    async->write((void*)buffer, NULL);

}

static void runOneInstance(Queue<Task> *mq, Summary *generalSummary, int instance_size, 
                              AsyncChannel *async, const int repetitionToComputeMeans, int max_iter)
{
    Task t;
    
    Summary localSummary;
    while(mq->ifhaspop(t))
    {
        int bestEvaluationAchievedHC, bestEvaluationAchievedSA, bestEvaluationAchievedPRSA;
        bestEvaluationAchievedSA = bestEvaluationAchievedPRSA = bestEvaluationAchievedHC = ZINF;

        char nome_da_instancia[300];
            
        int job_index;

        strcpy(nome_da_instancia,t.target);
        job_index = t.id;

        
        double alfa = 1;
        double beta = 50;
        char target[300] = "";
        int rc = 10;

        int i = job_index;

        char outputImage[20] = "";
        sprintf(outputImage,"image%d",i);

        sprintf(target,"%s",nome_da_instancia);
        strcat(target,(char*)".txt");
        if(rc == 0)
        {
            printf("Erro de leitura\n");
            exit(0);
        }
        
        char diretorioDasInstancias[200] = "";
        getCurrentFolder(diretorioDasInstancias, sizeof(diretorioDasInstancias));
        
        if(instance_size == SMALL_SIZE)
        {
            strcat(diretorioDasInstancias,"instances/small/");
        }
        else
        {
            strcat(diretorioDasInstancias,"instances/large/");
        }
        strcat(diretorioDasInstancias,target);
        
        Instancia inst(diretorioDasInstancias);
        
        //Solucao sdjasa = djasa(&inst,averageRec,alfa,beta);
        //double ava_djasa = sdjasa.avaliaSolucao(alfa,beta);

        double alfa_aleatoriedade = 0.5;

        for( int count = 0; count < repetitionToComputeMeans; ++count)
        {
            list<Sample> samplesHc;
            list<Sample> samplesPrsa;
            list<Sample> samplesSa;
            
            
            (void) grasp_with_setings(&inst, averageRec, alfa, beta, max_iter, 
                                                          alfa_aleatoriedade, &samplesHc, hc);
            if (samplesHc.back().evaluation < bestEvaluationAchievedHC)
            {
                bestEvaluationAchievedHC = samplesHc.back().evaluation;
            }

            (void) grasp_with_setings(&inst, averageRec, alfa, beta, max_iter, alfa_aleatoriedade, 
                                                                                &samplesPrsa, prsa);
            if (samplesPrsa.back().evaluation < bestEvaluationAchievedPRSA)
            {
                bestEvaluationAchievedPRSA = samplesPrsa.back().evaluation;
            }

            (void) grasp_with_setings(&inst, averageRec, alfa, beta, max_iter, alfa_aleatoriedade, 
                                                                                    &samplesSa, sa);
            if (samplesSa.back().evaluation < bestEvaluationAchievedSA)
            {
                bestEvaluationAchievedSA = samplesSa.back().evaluation;
            }

            localSummary.hc.push(samplesHc);  
            localSummary.prsa.push(samplesPrsa);
            localSummary.sa.push(samplesSa);
            //cout << "\t\tONE REPETITION COMPLETED!!!\n";

        }
        computeMeans(&localSummary);

        Solucao djsasaSolution = djasa(&inst, averageRec, alfa, beta );
        double avaDjasa = djsasaSolution.avaliaSolucao(alfa,beta);
        cout << "Ava djasa: " << avaDjasa << "\n";

        dump_results_structured(instance_size, target, outputImage, 
                          localSummary.meanHc, localSummary.meanPrsa, localSummary.meanSa, 
                         (char*)"Hill Climbing", (char*)"PRSA", (char*) "SA", t.current_best, 
                                                                                          avaDjasa);

        writeCsvLine(async, nome_da_instancia, localSummary.meanHc.back().evaluation, 
                                                                localSummary.meanHc.back().time, 
                                                                t.current_best, // on literature
                                                                bestEvaluationAchievedHC,
                                                                (char*)"Hill Climbing");

        writeCsvLine(async, nome_da_instancia, localSummary.meanPrsa.back().evaluation, 
                                                                localSummary.meanPrsa.back().time, 
                                                                t.current_best, // on literature
                                                                bestEvaluationAchievedPRSA,
                                                                (char*)"PRSA");

        writeCsvLine(async, nome_da_instancia, localSummary.meanSa.back().evaluation, 
                                                                localSummary.meanSa.back().time, 
                                                                t.current_best, // on literature
                                                                bestEvaluationAchievedSA,
                                                                (char*)"SA");

    }
}

static void runRefactored(Queue<Task> &mq, Summary &generalSummary, int instance_size,
                                                                    int firstIndex, int lastIndex, 
                                                                    int repetitions, int iterations)
{

    char outputResultsTxt[200];
    if( LARGE_SIZE == instance_size)
    {
        sprintf(outputResultsTxt,"%s", (char*) "autogenerated/large/resultsLarge.csv");
    }
    else if( SMALL_SIZE == instance_size)
    {
        sprintf(outputResultsTxt,"%s", (char*) "autogenerated/small/resultsSmall.csv");
    }
    else
    {
        printf("Invalid instance size\n");
        return;
    }

    AsyncChannel async((const char *)outputResultsTxt);
    createCsvTable(&async);

    static unsigned concurentThreadsSupported = std::thread::hardware_concurrency();
    
    char target[300] = "";
    double current_best;
    int rc = 0;
    
    char diretorioDasInstancias[200] = "";
    getCurrentFolder(diretorioDasInstancias, sizeof(diretorioDasInstancias));
    
    if(instance_size == SMALL_SIZE)
    {
        strcat(diretorioDasInstancias,"instances/small/");
    }
    else
    {
        strcat(diretorioDasInstancias,"instances/large/");
    }
    strcat(diretorioDasInstancias,target);
    
    char instances_index[300] = "";
    char optimal_values_index[300] = "";
    
    strcpy(instances_index, diretorioDasInstancias);
    strcat(instances_index, (char*)"indice_de_instancias");

    strcpy(optimal_values_index, diretorioDasInstancias);
    strcat(optimal_values_index, (char*)"indice_de_otimos");

    FILE *nomes_instancias = fopen(instances_index,"r");
    if(!nomes_instancias) 
    {
        printf("Fail to open %s\n",instances_index);
    }
    FILE *otimos_das_instancias = fopen(optimal_values_index,"r");
    if(!otimos_das_instancias) 
    {
        printf("Fail to open %s\n",optimal_values_index);
    }

    int quantidade_nomes, quantidade_otimos;
    rc = fscanf(nomes_instancias,"%d",&quantidade_nomes);
    rc = fscanf(otimos_das_instancias,"%d",&quantidade_otimos);

    if(quantidade_nomes != quantidade_otimos)
    {
        printf("Erro, arquivos com tamanhos diferentes!!!\n");
        return;
    }

    for(int i = 0; i < quantidade_nomes; i++)
    {
        char outputImage[20] = "";
        sprintf(outputImage,"image%d",i);

        rc = fscanf(nomes_instancias,"%s",target);
        if(rc == 0)
        {
            printf("Erro de leitura\n");
            exit(0);
        }
        rc = fscanf(otimos_das_instancias,"%lf",&current_best);
        if(rc == 0)
        {
            printf("Erro de leitura\n");
            exit(0);
        }

        Task t;
        t.id = i;
        strcpy(t.target,target);
        t.current_best = current_best;

        if (i >= firstIndex-1 && i <= lastIndex-1)
        {
            mq.push(t);
        }
        
    }
    
    
    list<thread> myThreads;

    for(int i = 0; i < (int) concurentThreadsSupported; i++)
    {
        myThreads.push_back(thread(runOneInstance,&mq, &generalSummary, instance_size, &async,
                                                                          repetitions, iterations));
    }

    for (list<thread>::iterator it = myThreads.begin(); it != myThreads.end(); ++it)
    {
        it->join();
    }

    fclose(nomes_instancias);
    fclose(otimos_das_instancias);
}

void BateriaDeTestes::run(int instance_size, int firstIndex, int lastIndex, 
                                                                    int repetitions, int iterations)
{
	runRefactored(mq, generalSummary, instance_size, firstIndex, lastIndex, 
                                                                           repetitions, iterations);
}

static int evaluateSequence(list<Sample> &seq, double time)
{
    if(time < seq.begin()->time) 
    {
        return seq.begin()->evaluation;
    }
    else
    {
        for(Sample s : seq)
        {
            if(s.time >= time)
            {
                return s.evaluation;
            }
        }
        return seq.end()->evaluation;
    }
}   

static void computeMaxMin(list< list <Sample> > &set, double *start, double *end)
{
    double tini = -1;
    double tfim = -1;
        
    for(list<Sample> lss: set)
    {
        if ( -1 == tini )
        {
            tini = lss.begin()->time;
        }
        else if ( lss.begin()->time > tini )
        {
            tini = lss.begin()->time;
        }

        if ( -1 == tfim )
        {
            tfim = lss.back().time;
        }
        else if ( lss.back().time < tfim )
        {
            tfim = lss.back().time;
        }
    }
    *end = tfim;
    *start = tini;
}

static void computeMean(Queue<list<Sample>> &set, list<Sample> &sampleMean)
{
    double tstart, tend;

    list<list<Sample>> set_cpy;
    list<Sample> aux;
    list<Sample> processedMean;

    while(set.ifhaspop(aux))
    {
        set_cpy.push_back(aux);
    }
    
    int cnt = 0;

    for( list<Sample> s : set_cpy)
    {
        ++cnt;
        if ( processedMean.size() == 0 )
        {
            processedMean = s;
        }
        else
        {
            processedMean = sumSamples(processedMean, s);
        }
    }
    multiplyByConstant(processedMean, 1/(double)cnt);
    sampleMean = processedMean;
}

void computeMeans(Summary *generalSummary)
{
    computeMean(generalSummary->hc, generalSummary->meanHc);
    computeMean(generalSummary->prsa, generalSummary->meanPrsa);
    computeMean(generalSummary->sa, generalSummary->meanSa);
}

BateriaDeTestes::~BateriaDeTestes() {
}

} /* namespace std */
