#include <iostream>
#include <fstream>
#include <vector>
#include<assert.h>
#include <omp.h>
#include <cmath>
#include <chrono>
#include <thread>

#define d(x) cerr<<#x<<" "<<x<<"\n"

using namespace std;

struct Pixel {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
};

#pragma pack(push, 1)
struct BMPHeader {
    char signature[2];
    int fileSize;
    int reserved;
    int dataOffset;
    int headerSize;
    int width;
    int height;
    short planes;
    short bitsPerPixel;
    int compression;
    int dataSize;
    int horizontalResolution;
    int verticalResolution;
    int colors;
    int importantColors;
};

#pragma pack(pop)

vector<vector<Pixel>> leerArchivoBMP(const char* nombreArchivo) {
    ifstream archivo(nombreArchivo, ios::binary);

    if (!archivo) {
        cerr << "No se pudo abrir el archivo BMP" << endl;
        exit(1);
    }

    BMPHeader header;
    archivo.read(reinterpret_cast<char*>(&header), sizeof(BMPHeader));

    if (header.bitsPerPixel != 24) {
        cerr << "El archivo BMP debe tener 24 bits por píxel" << endl;
        exit(1);
    }

    // Mover el puntero al inicio de los datos de píxeles
    archivo.seekg(header.dataOffset, ios::beg);

    vector<vector<Pixel>> matriz(header.height, vector<Pixel>(header.width));

    for (int i = 0; i < header.height; ++i) {
        for (int j = 0; j < header.width; ++j) {
            archivo.read(reinterpret_cast<char*>(&matriz[i][j]), sizeof(Pixel));
        }
        archivo.seekg(header.width % 4, ios::cur);
    }
    archivo.close();
    return matriz;
}


void guardarMatrizEnBMP(const char* nombreArchivo, const vector<vector<Pixel>>& matriz) {
    ofstream archivo(nombreArchivo, ios::binary);

    if (!archivo) {
        cerr << "No se pudo crear el archivo BMP" << endl;
        exit(1);
    }

    BMPHeader header;
    header.signature[0] = 'B';
    header.signature[1] = 'M';
    header.fileSize = sizeof(BMPHeader) + matriz.size() * ((3 * matriz[0].size()) + (matriz[0].size() % 4)) + 2; // +2 for padding
    header.reserved = 0;
    header.dataOffset = sizeof(BMPHeader);
    header.headerSize = 40;
    header.width = matriz[0].size();
    header.height = matriz.size();
    header.planes = 1;
    header.bitsPerPixel = 24;
    header.compression = 0;
    header.dataSize = matriz.size() * ((3 * matriz[0].size()) + (matriz[0].size() % 4)) + 2; // +2 for padding
    header.horizontalResolution = 0;
    header.verticalResolution = 0;
    header.colors = 0;
    header.importantColors = 0;

    archivo.write(reinterpret_cast<char*>(&header), sizeof(BMPHeader));

    // Escribir la matriz en el archivo BMP
    for (int i = 0; i < matriz.size(); ++i) {
        for (int j = 0; j < matriz[0].size(); ++j) {
            archivo.write(reinterpret_cast<const char*>(&matriz[i][j]), sizeof(Pixel));
        }
        // Rellenar con bytes de 0 para la alineación de 4 bytes
        for (int k = 0; k < matriz[0].size() % 4; ++k) {
            char paddingByte = 0;
            archivo.write(&paddingByte, 1);
        }
    }
    archivo.close();
}

inline Pixel oper(const Pixel &a, const Pixel &b){
    return {(a.blue + b.blue)/ 2, (a.green + b.green) / 2, (a.red + b.red) / 2};
}

vector<vector<Pixel>> sumarMatrices(vector<vector<Pixel>> &matriz1,vector<vector<Pixel>> &matriz2){
    int n = matriz1.size(), m = matriz1[0].size();
    vector<vector<Pixel>> resultante(n, vector<Pixel>(m));
    assert(m == matriz2[0].size());
    #pragma omp parallel for
    for(int i = 0; i < n; ++i){
        for(int j = 0; j < m; ++j){
            #pragma omp atomic
            resultante[i][j] = oper(matriz1[i][j], matriz2[i][j]);
        }
    }
    return resultante;
}

typedef long double ld;
typedef pair<int,int> ii;

ii mult(ii a, ld trans[2][2]){
    ld coord[2];
    for(int i = 0; i < 2; ++i){
        coord[i] = trans[i][0] * a.first  + trans[i][1] * a.second;
    }
    return {int(coord[0]), int(coord[1])};
}

bool inBounds(int i, int j, int &n, int &m){
    return i>=0 && j>= 0 && i < n && j < m;
}

vector<vector<Pixel>> rotarMatriz(vector<vector<Pixel>> &matriz, ld &theta, bool withOpenMP){
    int n = matriz.size(), m = matriz[0].size();
    ld trans[2][2] = {{cos(theta), sin(theta)}, {-sin(theta), cos(theta)}};
    vector<vector<Pixel>> mat(n, vector<Pixel> (m));
    int mx = n/2, my = m/2;
    #pragma omp parallel for
    for(int i = 0; i < n; ++i){
        for(int j = 0;j < m; ++j){
            #pragma omp atomic
            int ix = i - mx;
            int jx = j - my;
            ii cur = mult({ix, jx}, trans);
            cur.first += mx, cur.second += my;
            if(inBounds(cur.first, cur.second, n, m)){
                mat[i][j] = matriz[cur.first][cur.second];
            }
        }
    }
    return mat;
}

vector<vector<Pixel>> rotarMatriz(vector<vector<Pixel>> &matriz, ld &theta){
    int n = matriz.size(), m = matriz[0].size();
    ld trans[2][2] = {{cos(theta), sin(theta)}, {-sin(theta), cos(theta)}};
    vector<vector<Pixel>> mat(n, vector<Pixel> (m));
    int mx = n/2, my = m/2;
    for(int i = 0; i < n; ++i){
        for(int j = 0;j < m; ++j){
            int ix = i - mx;
            int jx = j - my;
            ii cur = mult({ix, jx}, trans);
            cur.first += mx, cur.second += my;
            if(inBounds(cur.first, cur.second, n, m)){
                mat[i][j] = matriz[cur.first][cur.second];
            }
        }
    }
    return mat;
}

void rotarRangoMatriz(
    vector<vector<Pixel>> &matriz,
    vector<vector<Pixel>> &resultante,
    int &n, int &m, int &mx, int &my, 
    ld &theta, ld trans[2][2],
    int startRow, int endRow){
    for(int i = startRow; i < endRow; ++i){
        for(int j = 0;j < m; ++j){
            int ix = i - mx;
            int jx = j - my;
            ii cur = mult({ix, jx}, trans);
            cur.first += mx, cur.second += my;
            if(inBounds(cur.first, cur.second, n, m)){
                resultante[i][j] = matriz[cur.first][cur.second];
            }
        }
    }
}

void add(int &a, int &b){
    a += b;
}

vector<vector<Pixel>> rotarMatrizHilos(vector<vector<Pixel>> &matriz, ld &theta, int numHilos){
    int n = matriz.size(), m = matriz[0].size();
    vector<vector<Pixel>> resultante(n, vector<Pixel> (m));

    int mx = n/2, my = m/2; 
    ld trans[2][2] = {{cos(theta), sin(theta)}, {-sin(theta), cos(theta)}};
    vector<thread> threads(numHilos);
    d(numHilos);
    for(int i = 0; i < numHilos; ++i){/// [) Inclusivo, abierto
        int startRow = i * (n / numHilos);
        int endRow = (i == numHilos - 1) ? n : (i+1) * (n / numHilos);
        threads[i] = thread(
            rotarRangoMatriz,
            ref(matriz),
            ref(resultante),
            ref(n),
            ref(m),
            ref(mx),
            ref(my),
            ref(theta),
            ref(trans),
            startRow,
            endRow);
    }
    for(int i = 0; i < numHilos; ++i){/// [) Inclusivo, abierto
        threads[i].join();
    }
    return resultante;
}

int convertstrToNum(const char *s)
{
  int sign=1;
  if(*s == '-'){
    sign = -1;
    s++;
  }
  int num=0;
  while(*s){
    num=((*s)-'0')+num*10;
    s++;   
  }
  return num*sign;
}

void trasladarRangoMatriz(vector<vector<Pixel>> &matriz,vector<vector<Pixel>> &resultante, int &x, int &y, int &n, int &m, int startRow, int endRow){

    for(int i = startRow; i < endRow; ++i){
        for(int j = 0;j < m; ++j){
            int ix = i + x;
            int jx = j + y;
            if(inBounds(ix, jx, n, m)){
                resultante[i][j] = matriz[ix][jx];
            }
        }
    }
}

vector<vector<Pixel>> trasladarMatrizHilos(vector<vector<Pixel>> &matriz, int x, int y, int numHilos){
    int n = matriz.size(), m = matriz[0].size();
    vector<vector<Pixel>> resultante(n, vector<Pixel> (m));
    vector<thread> threads(numHilos);
    // d(numHilos);
    for(int i = 0; i < numHilos; ++i){/// [) Inclusivo, abierto
        int startRow = i * (n / numHilos);
        int endRow = (i == numHilos - 1) ? n : (i+1) * (n / numHilos);
        threads[i] = thread(
            trasladarRangoMatriz, ref(matriz), ref(resultante), ref(x), ref(y), ref(n), ref(m), startRow, endRow
        );
    }
    for(int i = 0; i < numHilos; ++i){/// [) Inclusivo, abierto
        threads[i].join();
    }
    return resultante;
}


const ld  pi = acos(-1);

const int NUMCORES = 7;
int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Uso: " << argv[0] << " <nombre_del_archivo_entrada.bmp> <nombre_del_archivo_salida.bmp>" << endl;
        return 1;
    }
    const char* tipo = argv[1];
    ld angle = 20;
    auto start_time = chrono::steady_clock::now();

    const char* nombreArchivoLecturaBMP = argv[2];
    const char* nombreArchivoEscrituraBMP = argv[3];
    vector<vector<Pixel>> matriz = leerArchivoBMP(nombreArchivoLecturaBMP);
    d(tipo[0]);
    if(tipo[0] == 'r'){// Rotation
        ld theta = angle * pi / 180;
        // vector<vector<Pixel>> resultante = rotarMatriz(matriz1, theta);
        // vector<vector<Pixel>> resultante = rotarMatriz(matriz1, theta, 1);// 1 milisegundo más rápido que normal OPENMP
        vector<vector<Pixel>> resultante = rotarMatrizHilos(matriz, theta, NUMCORES);
        //32% mejora con 7 Hilos

        guardarMatrizEnBMP(nombreArchivoEscrituraBMP, resultante);
    }else if(tipo[0] == 't'){// Translation
        const char* xs = argv[4];
        const char* ys = argv[5];
        int x = convertstrToNum(xs), y = convertstrToNum(ys);
        x*=-1, y*=-1;
        d(x),d(y);
        vector<vector<Pixel>> resultante = trasladarMatrizHilos(matriz, x, y, NUMCORES);
        guardarMatrizEnBMP(nombreArchivoEscrituraBMP, resultante);
    }else if (tipo[0] == 's'){//Suma
        const char* nombreArchivoLecturaBMP2 = argv[4];
        d(nombreArchivoLecturaBMP2);
        vector<vector<Pixel>> matriz2 = leerArchivoBMP(nombreArchivoLecturaBMP2);
        vector<vector<Pixel>> resultante = sumarMatrices(matriz, matriz2);
        guardarMatrizEnBMP(nombreArchivoEscrituraBMP, resultante);
    }


    auto end_time = chrono::steady_clock::now();

    // Calculate the duration between the two time points
    auto duration = chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    cout<<"El algorítmo se demoró "<<(duration.count())<<" milisegundos\n";
    // Leer el archivo BMP y obtener la matriz de píxeles
    return 0;
}

/**
 * MEMORIA COMPARTIDA
 * 
 * Rotar
 * ./A r boat.bmp boatRotado.bmp         

 * 
 * SUMAR
 * ./A s boat.bmp fotoSumada.bmp Cascada.bmp

*/
