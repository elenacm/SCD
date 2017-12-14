#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

//**********************************************************************
// variables compartidas


const int num_items = 40,   // número de items
	       	tam_vec   = 10,   // tamaño del buffer
				 	num_prod = 5,
					num_cons = 5;
unsigned  cont_prod[num_items] = {0}, // contadores de verificación: producidos
          cont_cons[num_items] = {0}, // contadores de verificación: consumidos
					primera_libre = 0;
int buffer[tam_vec];
Semaphore puede_producir = tam_vec,
					puede_consumir = 0;
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

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato()
{
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "producido: " << contador << endl << flush ;

   cont_prod[contador] ++ ;
   return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "                  consumido: " << dato << endl ;

}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." ;
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  if ( cont_prod[i] != 1 )
      {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora(int inicio,int final)
{
  for( unsigned i = 0 ; i < final ; i+=num_prod)
  {
		int dato = producir_dato();
		sem_wait(puede_producir);

		m.lock();
		buffer[primera_libre++] = dato;
		m.unlock();

		sem_signal(puede_consumir);
  }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(int inicio,int final)
{
  for( unsigned i = 0 ; i < final ; i+=num_cons)
  {
		int dato;
		sem_wait(puede_consumir);

		m.lock();
		dato = buffer[--primera_libre];
		m.unlock();

		sem_signal(puede_producir);
		consumir_dato( dato );
  }
}
//----------------------------------------------------------------------

int main()
{
  cout << "---------------------------------------------------------" << endl
      << "Problema de los productores-consumidores (solución LIFO)." << endl
      << "---------------------------------------------------------" << endl
      << flush ;

  thread hebra_productora[num_prod],
				 hebra_consumidora[num_cons];

	for(int i=0; i<num_prod;i++)
		hebra_productora[i]=thread(funcion_hebra_productora, i, num_items);
	for(int i=0; i<num_cons;i++)
		hebra_consumidora[i]=thread(funcion_hebra_consumidora, i, num_items);

	for(int i=0; i<num_prod;i++)
  	hebra_productora[i].join();
	for(int i=0; i<num_cons;i++)
  	hebra_consumidora[i].join();

  test_contadores();

	return 0;
}
