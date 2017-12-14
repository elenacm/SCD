// -----------------------------------------------------------------------------
// Archivo: prodcons2-mu.cpp
// Implementación del problema del productor-consumidor con
// un proceso intermedio que gestiona un buffer finito y recibe peticiones
// en orden arbitrario
// (versión con múltiples productores y múltiples consumidores)
// -----------------------------------------------------------------------------

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   num_prod              = 4 ,
   num_cons              = 5 ,
   id_productor          = num_prod - 1,
   id_buffer             = num_prod,
   id_consumidor         = num_prod + num_cons - 1,
   tag_productor         = 0,
   tag_consumidor        = 1,
   num_procesos_esperado = 10 ,
   num_items             = 20,
   tam_vector            = 10;

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
// producir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio
int producir(int num_orden){
   static int contador = num_orden*(num_items/num_prod);
   sleep_for( milliseconds( aleatorio<10,100>()) );
   contador++ ;
   cout << "Productor " << num_orden << " ha producido valor " << contador << endl << flush;
   return contador ;
}
// ---------------------------------------------------------------------

bool EsProductor(int prod){
  return (prod >= 0 && prod <= id_productor);
}

bool EsConsumidor(int cons){
  return (cons > num_prod && cons < num_prod+num_cons);
}

bool EsBuffer(int b){
  return (b == num_prod);
}

// ---------------------------------------------------------------------
void funcion_productor(int num_orden){
   for ( unsigned int i= 0 ; i < num_items/num_prod ; i++ )
   {
      // producir valor
      int valor_prod = producir(num_orden);
      // enviar valor
      cout << "Productor " << num_orden << " va a enviar valor " << valor_prod << endl << flush;
      MPI_Ssend( &valor_prod, 1, MPI_INT, id_buffer, tag_productor, MPI_COMM_WORLD );
   }
}
// ---------------------------------------------------------------------

void consumir( int valor_cons,int num_orden ){
   // espera bloqueada
   sleep_for( milliseconds( aleatorio<110,200>()) );
   cout << "Consumidor " << num_orden << " ha consumido valor " << valor_cons << endl << flush ;
}
// ---------------------------------------------------------------------

void funcion_consumidor(int num_orden){
   int         peticion,
               valor_rec = 1 ;
   MPI_Status  estado ;

   for( unsigned int i=0 ; i < num_items/num_cons; i++ )
   {
      MPI_Ssend( &peticion,  1, MPI_INT, id_buffer, tag_consumidor, MPI_COMM_WORLD);
      MPI_Recv ( &valor_rec, 1, MPI_INT, id_buffer, 2, MPI_COMM_WORLD, &estado);
      cout << "Consumidor " << num_orden << " ha recibido valor " << valor_rec << endl << flush ;
      consumir( valor_rec, num_orden );
   }
}
// ---------------------------------------------------------------------

void funcion_buffer(int num_orden){
   int        buffer[tam_vector],      // buffer con celdas ocupadas y vacías
              valor,                   // valor recibido o enviado
              primera_libre       = 0, // índice de primera celda libre
              primera_ocupada     = 0, // índice de primera celda ocupada
              num_celdas_ocupadas = 0, // número de celdas ocupadas
              tag_emisor_aceptable ;    // identificador de emisor aceptable
   MPI_Status estado ;                 // metadatos del mensaje recibido

   for( unsigned int i=0 ; i < num_items*2 ; i++ )
   {
      // 1. determinar si puede enviar solo prod., solo cons, o todos

      if ( num_celdas_ocupadas == 0 )               // si buffer vacío
         tag_emisor_aceptable = 0;       // $~~~$ solo prod.
      else if ( num_celdas_ocupadas == tam_vector ) // si buffer lleno
         tag_emisor_aceptable = 1 ;      // $~~~$ solo cons.
      else                                          // si no vacío ni lleno
         tag_emisor_aceptable = MPI_ANY_TAG ;     // $~~~$ cualquiera

      // 2. recibir un mensaje del emisor o emisores aceptables

      MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, tag_emisor_aceptable, MPI_COMM_WORLD, &estado );

      // 3. procesar el mensaje recibido

      switch( estado.MPI_TAG ) // leer emisor del mensaje en metadatos
      {
         case tag_productor: // si ha sido el productor: insertar en buffer
            buffer[primera_libre] = valor ;
            primera_libre = (primera_libre+1) % tam_vector ;
            num_celdas_ocupadas++ ;
            cout << "Buffer ha recibido valor " << valor << endl ;
            break;

         case tag_consumidor: // si ha sido el consumidor: extraer y enviarle
            valor = buffer[primera_ocupada] ;
            primera_ocupada = (primera_ocupada+1) % tam_vector ;
            num_celdas_ocupadas-- ;
            cout << "Buffer va a enviar valor " << valor << endl ;
            MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE, 2, MPI_COMM_WORLD);
            break;
      }
   }
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
   int id_propio, num_procesos_actual;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos_esperado == num_procesos_actual )
   {
      // ejecutar la operación apropiada a 'id_propio'
      if ( EsProductor(id_propio) )
         funcion_productor(id_propio);
      else if ( EsBuffer(id_propio) )
         funcion_buffer(id_propio);
      else
         funcion_consumidor(id_propio);
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize( );
   return 0;
}
