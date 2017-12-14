#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

const int tam_vec = 3;
Semaphore libre = 1,
          buffer[tam_vec] = {0, 0, 0},
          envio_suministro = 1,
          producir_suministro = 0;
mutex m;


//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero
int producir(){
  return aleatorio<0,tam_vec-1>();
}

void funcion_hebra_estanquero(){
  int dato;
  int suministro = 0;
  while(true){
    if(suministro == 0){
      sem_signal(producir_suministro);
      sem_wait(envio_suministro);
      suministro = 10;
    }
    int dato = producir();
    suministro--;
    sem_wait(libre);
    m.lock();
    cout << "Estanquero produce ingrdiente: " << dato << endl;
    cout << "Suministros: " << suministro << endl;
    m.unlock();
    sem_signal(buffer[dato]);
  }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar
   m.lock();
    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
   m.unlock();
   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar
   m.lock();
    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
   m.unlock();
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   while( true )
   {
     sem_wait(buffer[num_fumador]);
     m.lock();
     cout << "Fumador " << num_fumador << " retira su ingrediente." << endl;
     m.unlock();
     sem_signal(libre);
     fumar(num_fumador);
   }
}

//----------------------------------------------------------------------
//función que ejecuta la hebra de la entidad suminstradora
void funcion_hebra_suministradora(){
  while(true){
    sem_wait(producir_suministro);
    m.lock();
    cout << "Nuevo Suministro" << endl;
    m.unlock();
    sem_signal(envio_suministro);
  }
}
//----------------------------------------------------------------------

int main()
{
   thread hebra_estanquero(funcion_hebra_estanquero),
          hebra_fumador[tam_vec];

    for(int i=0; i < tam_vec; i++)
      hebra_fumador[i]=thread(funcion_hebra_fumador, i);

    hebra_estanquero.join();

    for(int i=0; i < tam_vec; i++)
      hebra_fumador[i].join();

    cout << "FIN" << endl;

    return 0;
}
