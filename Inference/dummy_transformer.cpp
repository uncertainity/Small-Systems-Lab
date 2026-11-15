#include <iostream>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <cmath>
using namespace std;

//// Structs 

struct TokenizerSettings{
    unordered_map<char,int> char_to_id;
    vector<char>id_to_char;
};

struct EmbeddingsTable{
    int vocab_size;
    int d_model;
    vector<vector<double>> embWeights;
};

struct DummyWeights{
    vector<vector<double>> Wq;
    vector<vector<double>> Wk;
    vector<vector<double>> Wv;
    vector<vector<double>> Wout; // d_model x vocab_size
};


//// Pretty Prints and Debug Prints

void printVocabEmbeddings(
    vector<char>& id_to_char,
    const vector<vector<double>>& embWeights
){
    cout << "\n=== Embeddings ===\n";

    for(int i = 0; i < id_to_char.size(); i++){

        cout << id_to_char[i] << " : [ ";

        for(int j = 0; j < embWeights[i].size(); j++){

            cout << fixed << setprecision(4)
                 << embWeights[i][j];

            if(j != embWeights[i].size() - 1)
                cout << ", ";
        }

        cout << " ]\n";
    }

    cout << endl;
}

void printTokenizerEmbeddings(
    string s,
    const vector<vector<double>>& embeddings
) {
    cout << "\n=== Embeddings ===\n";

    for (int i = 0; i < embeddings.size(); i++) {
        cout << s[i] << " : [ ";

        for (int j = 0; j < embeddings[i].size(); j++) {
            cout << embeddings[i][j];

            if (j != embeddings[i].size() - 1)
                cout << ", ";
        }

        cout << " ]\n";
    }

    cout << endl;
}


template<typename T>
void print1d(vector<T> &arr){
    for(int i = 0; i < arr.size(); i++){
        cout<<arr[i] << ",";
    }
    cout<<"\n";
}


/// Initialiazing Dummy Tokenizers, Embedding weights

TokenizerSettings createTokenizer(){
    TokenizerSettings ts;
    for (char c = 'a'; c <= 'z';c++){
        int id = c - 'a';
        ts.char_to_id[c] = id;
        ts.id_to_char.push_back(c);
    }
    // EOS "." and PAD "/"
    // ts.char_to_id['/']  = 27;
    // ts.char_to_id['.'] = 28;
    // ts.id_to_char.push_back('/');
    // ts.id_to_char.push_back('.');
    return ts;
}

EmbeddingsTable createEmbeddingsTable(){
    EmbeddingsTable embeddingsTable;
    embeddingsTable.vocab_size = 26;
    embeddingsTable.d_model = 5;
    embeddingsTable.embWeights.resize(embeddingsTable.vocab_size,vector<double>(embeddingsTable.d_model,0.0));
    for(int i = 0; i < embeddingsTable.embWeights.size(); i++){
        for(int j = 0; j < embeddingsTable.embWeights[0].size();j++){
            embeddingsTable.embWeights[i][j] = ((double)rand()/RAND_MAX - 0.5);
        }
    }
    return embeddingsTable;
}

DummyWeights createDummyWeights(int d_model, int vocab_size){
    DummyWeights dw;

    dw.Wq.resize(d_model, vector<double>(d_model, 0.0));
    dw.Wk.resize(d_model, vector<double>(d_model, 0.0));
    dw.Wv.resize(d_model, vector<double>(d_model, 0.0));
    dw.Wout.resize(d_model, vector<double>(vocab_size, 0.0));

    for(int i = 0; i < d_model; i++){
        for(int j = 0; j < d_model; j++){

            dw.Wq[i][j] =
                ((double)rand()/RAND_MAX - 0.5);

            dw.Wk[i][j] =
                ((double)rand()/RAND_MAX - 0.5);

            dw.Wv[i][j] =
                ((double)rand()/RAND_MAX - 0.5);
        }
    }

    for(int i = 0; i < d_model;i++){
        for(int j = 0; j < vocab_size;j++){
            dw.Wout[i][j] = ((double)rand()/RAND_MAX - 0.5);
        }
    }

    return dw;
}


//// Transformer Helpers 
vector<vector<double>> matmult(
    const vector<vector<double>>& A,
    const vector<vector<double>>& B
){
    int n = A.size();
    int m = A[0].size();
    int p = B[0].size();

    vector<vector<double>> C(n, vector<double>(p, 0.0));

    for(int i = 0; i < n; i++){
        for(int k = 0; k < p; k++){
            for(int j = 0; j < m; j++){
                C[i][k] += A[i][j] * B[j][k];
            }
        }
    }

    return C;
}


vector<vector<double>>transpose(vector<vector<double>> &arr1){
    vector<vector<double>> arr2(arr1[0].size(),vector<double>(arr1.size(),0));
    for(int i = 0; i < arr1.size();i++){
        for(int j = 0; j < arr1[0].size();j++){
            arr2[j][i] = arr1[i][j];
        }
    }
    return arr2;
}

vector<vector<double>> rowSoftmax(vector<vector<double>> &scores){
    for(int i = 0; i < scores.size(); i++){
        double mx = *max_element(scores[i].begin(), scores[i].end());

        double sum = 0.0;

        for(int j = 0; j < scores[i].size(); j++){
            scores[i][j] = exp(scores[i][j] - mx);
            sum += scores[i][j];
        }

        for(int j = 0; j < scores[i].size(); j++){
            scores[i][j] /= sum;
        }
    }

    return scores;
}

vector<double> softmax(vector<double> &logits){
    double mx = *max_element(logits.begin(), logits.end());

    double sum = 0.0;

    for(int i = 0; i < logits.size(); i++){
        logits[i] = exp(logits[i] - mx);
        sum += logits[i];
    }

    for(int i = 0; i < logits.size(); i++){
        logits[i] /= sum;
    }

    return logits;
}

vector<double> matvec(vector<double> &vec1,vector<vector<double>> &arr1){
    vector<double>vec2(arr1[0].size(),0);
    for(int i = 0; i < arr1[0].size();i++){
        for(int j = 0;j < vec1.size(); j++){
            vec2[i] += (arr1[j][i] * vec1[j]);
        }
    }
    return vec2;
}

double dotProduct(vector<double> &vec1, vector <double> &vec2){
    double ans = 0.0;
    for(int i = 0; i < vec1.size(); i++) ans += (vec1[i] * vec2[i]);
    return ans;
}

/// Dummy Transformers 

vector<double> dummyTransformerTraining(vector<vector<double>> cur_embs,DummyWeights dw){
    vector<vector<double>> K = matmult(cur_embs,dw.Wk);

    // You dont need all the Q values
    vector<vector<double>> Q = matmult(cur_embs,dw.Wq);
    //vector<vector<double>>Q_last = matmult(cur_embs[cur_embs.size() - 1],dw.Wq);
    vector<vector<double>> V = matmult(cur_embs,dw.Wv);
    vector<vector<double>> KT = transpose(K);
    double scale = sqrt((double)dw.Wk.size());
    vector<vector<double>> scores = matmult(Q,KT);
    for(int i = 0; i < scores.size(); i ++){
        for(int j = 0; j < scores.size(); j++){
            scores[i][j] = scores[i][j]/scale;
            if(j>i) scores[i][j] = 1e-9; //basically inf
        }
        
    }
    vector<vector<double>> attn = rowSoftmax(scores);
    vector<vector<double>> context = matmult(attn, V);
    vector<double> last_context = context[context.size() - 1];
    vector<double> logits = matvec(last_context, dw.Wout);
    vector<double> probs = softmax(logits);
    return probs;
};

vector<double> dummyTransformerInference(vector<vector<double>> cur_embs,DummyWeights dw){
    int seq_len = cur_embs.size();
    int d_model = cur_embs[0].size();

    vector<vector<double>> K = matmult(cur_embs,dw.Wk); //(seq_len x d_model)*(d_model x d_model) -> (seq_lem x d_model) 
    vector<vector<double>> V = matmult(cur_embs,dw.Wv); //(seq_len x d_model)*(d_model x d_model) -> (seq_lem x d_model)
    vector<double> q_last = matvec(cur_embs[cur_embs.size() - 1],dw.Wq); // (1 x d_model) * (d_model x d_model) -> (1 x d_model)
    //vector<vector<double>> KT = transpose(K);
    
    vector<double> scores(seq_len, 0.0);
    for (int i = 0; i < seq_len;i++){
        scores[i] = dotProduct(q_last,K[i])/sqrt(double(d_model));
    } // scores -> (1 x d_model) * (d_model x seq_len) -> (1 x seq_len)
    vector<double> attn = softmax(scores); // (1 x seq_len)
    vector<double> context = matvec(attn,V); // (1 x seq_len) * (seq_len * d_model) -> (1 x d_model)
    vector<double> logits = matvec(context, dw.Wout); // (1 x d_model) * (d_model x vocab_size) -> (1 x vocab_size)
    vector<double> probs = softmax(logits); // (1 x vocab_size)
    return probs;
}


vector<vector<double>> getEmbeddings(string s,EmbeddingsTable emb, TokenizerSettings ts){
    vector<vector<double>> cur_embs(s.size(),vector<double>(emb.d_model,0));
    for(int i = 0; i < s.size(); i++){
        int id = ts.char_to_id.at(s[i]);
        cur_embs[i] = emb.embWeights[id];
    }
    return cur_embs;
}

int argmax(vector<double>& probs){
    int idx = 0;

    for(int i = 1; i < probs.size(); i++){
        if(probs[i] > probs[idx]){
            idx = i;
        }
    }

    return idx;
}


int main(){
    
    int MAX_INPUT_TOKENS = 25; // you don't need this if KV cache isn't there.
    int MAX_OUTPUT_TOKENS = 5;

    TokenizerSettings ts = createTokenizer();
    cout<<"Tokenizer Created;" << endl;
    EmbeddingsTable emb = createEmbeddingsTable();
    cout<<"Vocab Embeddings Created;" << endl;
    DummyWeights dw = createDummyWeights(emb.d_model,emb.vocab_size);
    cout<<"Dummy Weights Created."<<endl;
    printVocabEmbeddings(ts.id_to_char,emb.embWeights);
    string s = "aaabcde";
    vector<vector<double>> cur_embs = getEmbeddings(s,emb,ts);
    printTokenizerEmbeddings(s,cur_embs);
    cout<<"Embeddings for the input is generated"<<endl;
    string gens;
    for(int step = 0; step < MAX_OUTPUT_TOKENS; step ++){
        vector<double>probs = dummyTransformerInference(cur_embs,dw);
        //print1d(probs);
        int next_id = argmax(probs);
        //cout<<"Argmax:"<<next_id<<endl;
        char next_char = ts.id_to_char[next_id];
        gens.push_back(next_char);
        cur_embs.push_back(emb.embWeights[next_id]);
    }
    cout<<"Original String:"<<s<<endl;
    cout<<"Generated String:"<<gens<<endl;
}




