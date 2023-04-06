#ifndef COMPORTAMIENTOJUGADOR_H
#define COMPORTAMIENTOJUGADOR_H

#include "comportamientos/comportamiento.hpp"
using namespace std;

struct state{
  int row;
  int col;
  Orientacion compass;
};

class ComportamientoJugador : public Comportamiento{
  private:
  // Declarar aquí las variables de estado

  // Booleanos true si conocemos nuestra posicion, tenemos
  // zapato y bikini respectivamente
  bool position_known, shoes_on, bikini_on;
  // Booleanos que permiten pasar por bosque o agua
  bool forest_allowed, water_allowed;
  // Booleano que indica que estoy siguiendo un objeto con prioridad
  bool follow_priority;
  // Booleanos que indican que estoy abrazando muro, que lo dejo,
  // que me he movido después de colisionar con muro, y si la dirección
  // en la que giro siempre al colisionar es la derecha
  // puntos que indican donde empezamos a abrazarlo, el punto donde 
  // nos despegamos del muro, y el destino
  bool follow_wall, leave_wall, moved_after_hitting, follow_right;
  // Booleana para un caso muy especial cuando tenemos que rodear un 
  // muro en diagonal (hace falta dibujo para entenderlo)
  bool hard_turn;
  pair<int,int> hit_point, leave_point, target_point;
  // Orientación inicial al chocar (para saber si estoy encerrado)  
  Orientacion init_ori;
  // Punto de prioridad
  pair<int,int> priority_point;
  // Entero con el número de turnos desde la última recarga
  int turns_without_charging;

  // Estado actual
  state curr_state;
  // Última acción
  Action last_action;
  // Dimensiones del mapa
  int map_length, map_width;
  // Mapa auxiliar para anotar lo explorado mientras se desconoce la posición actual
  vector<vector<unsigned char>> aux_map;  
  // Posiciones de boosters guardados durante todas las vidas
  vector<pair<int,int>> shoes_locations, bikini_locations;

  // Inicialización del agente, útil para el constructor y respawn
  void initialize() {
    position_known = false;
    shoes_on = false;
    bikini_on = false;
    forest_allowed = false;
    water_allowed = false;
    follow_wall = false;
    leave_wall = false;
    moved_after_hitting = false;
    follow_right = true;
    hard_turn = false;
    init_ori = norte;
    follow_priority = false;
    priority_point = {-1,-1};
    hit_point = {-1,-1};
    leave_point = {-1,-1};
    target_point = {-1,-1};
    //turns_without_charging = 0;
    map_width = mapaResultado.size();
    map_length = map_width > 0 ? mapaResultado[0].size() : 0;
    aux_map.resize(2*map_width-1, vector<unsigned char>(2*map_length-1));
    curr_state.col = map_length-1;  
    curr_state.row = map_width-1;
    curr_state.compass = norte;
    last_action = actIDLE;    
    // Rellenar matrix auxiliar
    for (auto & row : aux_map) {
      for (auto & el : row) {
        el = '?';
      }
    }
    // Rellenar matriz resultado con precipicios
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < map_length; ++j) {
        mapaResultado[i][j] = 'P';
        mapaResultado[map_width-i-1][j] = 'P';
      }
      for (int j = 0; j < map_width; ++j) {
        mapaResultado[j][i] = 'P';
        mapaResultado[j][map_length-i-1] = 'P';
      }
    }
  }

  // Función auxiliar que devuelve las coordenadas en el mapa de la celda
  // número n(0..15) en el campo de visión actual
  pair<int,int> getCoordinates(int n);

  // Función auxiliar que actualiza el mapa auxiliar o el mapa resultado
  // (dependiendo de si conocemos nuestra posición) con información sobre
  // el terreno que vemos en el momento dado
  void updateMap(vector<vector<unsigned char>> & v, Sensores sens);

  // Función auxiliar que inserta posición si noestá en el vector de posiciones
  void addPosToVector(vector<pair<int,int>> & v, pair<int,int> p);

  // Función auxiliar que transfiere el contenido del mapa auxiliar al mapa
  // resultado tras haber encontrado la posición del agente
  void transferToAnswerMap(Sensores sens);
  
  // Función auxiliar que calcula las coordenadas de la celda '?' sin explorar más cercana 
  pair<int,int> nearestUnexploredCell();

  // Función auxiliar que devuelve true si el caracter es obstáculo
  bool isObstacle(unsigned char c);  

  // Función auxiliar que devuelve true si las casillas inmediatamente 
  // delante son '?'
  bool frontUnexplored();

  // Función auxiliar que devuelve la acción óptima para alcanzar
  // el punto dado 
  Action optimalMove(pair<int,int> & p);

  // Función auxiliar que devuelve la distancia al cuadrado  entre puntos
  int distanceBetween(pair<int,int> & p1, pair<int,int> & p2);

  // Función auxiliar que devuelve true si renta cargar baterías
  bool worthCharging(Sensores sens);

  // Función auxiliar que devuelve true si en el campo de visión hay
  // un objeto de interés y en tal caso le pone sus coordenadas a p
  bool prioritySpotted(Sensores sens, pair<int,int> & p);

  // Función auxiliar que devuelve true si es preferible esperar un turno
  // en vez de ir hacia delante
  bool avoidNPC(Sensores sens);

  // Función auxiliar que devuelve true si para llegar al punto p dado
  // es preferible girar a la derecha para abrazar el muro
  bool followWallRight(pair<int,int> p);

  public:
    ComportamientoJugador(unsigned int size) : Comportamiento(size){
      // Constructor de la clase
      // Dar el valor inicial a las variables de estado
      initialize();
      turns_without_charging = 0;
    }

    ComportamientoJugador(const ComportamientoJugador & comport) : Comportamiento(comport){}
    ~ComportamientoJugador(){}

    Action think(Sensores sensores);
    int interact(Action accion, int valor);
};

#endif
