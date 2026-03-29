#include <bits/stdc++.h>
using namespace std;

/* Não faz parte do projeto, é só pra gerar a lista de pontos pra teste*/
int main(){
    int n = 10000;
    for(int i = 0; i<n; i++){
        cout<<"POINT ";
        float x = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/1000.0));
        float y = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/1000.0));
        cout<<x<<" "<<y<<endl;
    }
}