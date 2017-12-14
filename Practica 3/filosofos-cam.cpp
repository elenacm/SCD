// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: filosofos-plantilla.cpp
// Implementación del problema de los filósofos (sin camarero).
// Plantilla para completar.
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------


#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   num_filosofos = 5 ,
   num_procesos  = 2*num_filosofos+1,
   etiq_coger = 0,
   etiq_soltar = 1,
   etiq_sentarse = 2,
   etiq_levantarse = 3,
   camarero = 0;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio(){
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

// ---------------------------------------------------------------------

void funcion_filosofos( int id ){
  int id_ten_izq = (id+1) % num_procesos, //id. tenedor izq.
      id_ten_der = (id-1); //id. tenedor der.
  int valor = 0;

  if (id_ten_izq == 0)
    id_ten_izq = 1;

  while ( true ){

    //Solicita sentarse
    cout <<"Filósofo " <<id << " solicita sentarse " <<endl << flush;
    MPI_Ssend(&valor, 1, MPI_INT, camarero, etiq_sentarse, MPI_COMM_WORLD);

    //Solicita tenedores
    cout << "Filósofo " << id << " solicita tenedor izquierdo " << id_ten_izq << endl << flush;
    MPI_Ssend(&valor, 1, MPI_INT, id_ten_izq, etiq_coger, MPI_COMM_WORLD);

    cout << "Filósofo " << id << " solicita tenedor derecho " << id_ten_der << endl << flush;
    MPI_Ssend(&valor, 1, MPI_INT, id_ten_der, etiq_coger, MPI_COMM_WORLD);

    //Comer
    cout << "Filósofo " << id << " comienza a COMER" << endl << flush;
    sleep_for( milliseconds( aleatorio<10,100>() ) );

    //Suelta tenedores
    cout << "Filósofo " << id << " suelta tenedor izquierdo " << id_ten_izq << endl << flush;
    MPI_Ssend(&valor, 1, MPI_INT, id_ten_izq, etiq_soltar, MPI_COMM_WORLD);

    cout<< "Filósofo " << id << " suelta tenedor derecho " << id_ten_der << endl << flush;
    MPI_Ssend(&valor, 1, MPI_INT, id_ten_der, etiq_soltar, MPI_COMM_WORLD);
    //Se levanta
    cout << "Filósofo " << id << " se levanta " << endl << flush;
    MPI_Ssend(&valor, 1, MPI_INT, camarero, etiq_levantarse, MPI_COMM_WORLD);

    cout << "Filosofo " << id << " comienza a PENSAR" << endl << flush;
    sleep_for( milliseconds( aleatorio<10,100>() ) );
 }
}
// ---------------------------------------------------------------------

void funcion_tenedores( int id ){
  int valor, id_filosofo ;  // valor recibido, identificador del filósofo
  MPI_Status estado ;       // metadatos de las dos recepciones

  while ( true ){
     MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_coger, MPI_COMM_WORLD, &estado);
     id_filosofo = estado.MPI_SOURCE;
     cout << "Tenedor " << id << " ha sido cogido por filósofo " << id_filosofo << endl << flush;

     MPI_Recv(&valor, 1, MPI_INT, id_filosofo, etiq_soltar, MPI_COMM_WORLD, &estado);
     cout << "Tenedor " << id << " ha sido liberado por filósofo " << id_filosofo << endl << flush;
  }
}

//----------------------------------------------------------------------

void funcion_camarero(){
  int valor, etiq_aceptable, sentados = 0;
  MPI_Status estado;

  while (true){
    if(sentados < num_filosofos-1){
      etiq_aceptable = MPI_ANY_TAG;
    }
    else{
      etiq_aceptable = etiq_levantarse;
    }

     MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_aceptable, MPI_COMM_WORLD, &estado);

     if (estado.MPI_TAG == etiq_sentarse){ // se le deja sentarse
         sentados++;
         cout << "Filosofo " << estado.MPI_SOURCE << " se sienta. Hay " << sentados << " filosofos sentados. " << endl << flush;
     }
     else if (estado.MPI_TAG == etiq_levantarse){ // Se levanta
         sentados--;
         cout << "Filosofo " << estado.MPI_SOURCE << " se levanta. Hay " << sentados << " filosofos sentados.  " << endl << flush;
     }
     else{
       cout << "El camarero ha recibido un mensaje erróneo " << endl << flush;
       exit(1);
     }
 }
}

// ---------------------------------------------------------------------

int main( int argc, char** argv ){
   int id_propio, num_procesos_actual ;

   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


   if ( num_procesos == num_procesos_actual ){
      // ejecutar la función correspondiente a 'id_propio'
      if(id_propio == 0)
        funcion_camarero();
      else if ( id_propio % 2 == 0 )          // si es par
         funcion_filosofos( id_propio ); //   es un filósofo
      else                               // si es impar
         funcion_tenedores( id_propio ); //   es un tenedor
   }
   else{
      if ( id_propio == 0 ){ // solo el primero escribe error, indep. del rol
        cout << "el número de procesos esperados es:    " << num_procesos << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   MPI_Finalize( );
   return 0;
}
