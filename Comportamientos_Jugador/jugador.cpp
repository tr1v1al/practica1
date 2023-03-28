#include "../Comportamientos_Jugador/jugador.hpp"
#include <iostream>
#include <algorithm>
using namespace std;


Action ComportamientoJugador::think(Sensores sensores){

	Action accion = actIDLE;

	// Actualizamos estado en función de los sensores y la última acción
	
	if (sensores.reset) {	// Agente muerto, respawn en punto random, inicializamos
		initialize();
	} else if (!sensores.colision) {	// Agente no choca, última acción exitosa
										// Si choca se quedara en bucle colisionando
		// Procesamos acción
		if (last_action == actFORWARD) {
			switch (curr_state.compass) {
				case norte: --curr_state.row; break;
				case noreste: --curr_state.row; ++curr_state.col; break;
				case este: ++curr_state.col; break;
				case sureste: ++curr_state.row; ++curr_state.col; break;
				case sur: ++curr_state.row; break;
				case suroeste: ++curr_state.row; --curr_state.col; break;
				case oeste: --curr_state.col; break;
				case noroeste: --curr_state.row; --curr_state.col; break;			
			}
		} else if (last_action != actIDLE){
			int temp = curr_state.compass;
			switch (last_action) {
			case actTURN_SL:
				temp = (temp+7)%8;	// Mismo que a-1+8
				break;
			case actTURN_SR:
				temp = (temp+1)%8;
				break;
			case actTURN_BL:
				temp = (temp+5)%8;	// Mismo que a-3+8
				break;
			case actTURN_BR:
				temp = (temp+3)%8;
				break;
			}
			curr_state.compass = static_cast<Orientacion>(temp);
		}
	}

	// Caso particular del nivel 0
	if (sensores.nivel == 0) {
		curr_state.row = sensores.posF;
		curr_state.col = sensores.posC;
		curr_state.compass = sensores.sentido;
		position_known = true;		
	} 	

	// Actualizamos estado si encontramos booster
	// En caso de estar posicionado, añadimos la posición del booster
	// al vector que almacena sus posiciones para futuras vidas

	// Si estabamos siguiendo un punto de prioridad y en el que estamos es
	// de ese tipo dejamos de seguir

	if (follow_priority) {
		unsigned char c = position_known ? 
		mapaResultado[priority_point.first][priority_point.second] : 
		aux_map[priority_point.first][priority_point.second];
		if (sensores.terreno[0] == c) {
			follow_priority = false;			
		}		
	}

	switch(sensores.terreno[0]) {
		// Si encontramos posición
		case 'G':
			if (!position_known) {
				// Transferimos lo explorado al resultado
				transferToAnswerMap(sensores);
				// Si seguimos punto de prioridad que no es G
				// tenemos que actualizar sus coordenadas a las del
				// mapa resultado
				if (follow_priority) {
					priority_point.first += sensores.posF - curr_state.row;
					priority_point.second += sensores.posC - curr_state.col;
				}
				// Pasamos de coordenadas del mapa auxiliar a las
				// del mapa resultado
				curr_state.row = sensores.posF;
				curr_state.col = sensores.posC;
				position_known = true;
				// Dejo de seguir muro
				follow_wall = false;
			}
			break;
		// Si encontramos zapatos
		case 'D':
			if (!shoes_on) {
				// Si encontramos zapatos dejamos de seguir muro
				shoes_on = true;
				forest_allowed = true;	
				follow_wall = false;
			}
			if (position_known) {
				addPosToVector(shoes_locations, {curr_state.row, curr_state.col});
			}
			break;
		// Si encontramos bikini
		case 'K':
			if (!bikini_on) {
				bikini_on = true;
				water_allowed = true;
				follow_wall = false;
			}
			if (position_known) {
				addPosToVector(bikini_locations, {curr_state.row, curr_state.col});
			}
			break;
	}

	// Actualizamos matriz
	updateMap(position_known ? mapaResultado : aux_map, sensores);
	
	// COMPORTAMIENTO GENERAL
	unsigned char izda = sensores.terreno[1], frente = sensores.terreno[2],
	dcha = sensores.terreno[3];
	// Incrementamos turnos sin cargar
	++turns_without_charging;

	// Vemos si podemos retirar permiso de paso por agua/bosque
	if (!shoes_on && forest_allowed || !bikini_on && water_allowed) {
		bool old_forest = forest_allowed, old_water = water_allowed;
		water_allowed = bikini_on;
		forest_allowed = shoes_on;
		if (isObstacle(dcha) && isObstacle(frente) && isObstacle(izda)) {
			water_allowed = old_water;
			forest_allowed = old_forest;		
		}
	}

	// Si no estamos siguiendo punto de prioridad, vemos si hay alguno
	if (!follow_priority) {
		pair<int,int> p = {-1,-1}; 
		if (prioritySpotted(sensores, p)) {
			follow_priority = true;
			priority_point = p;
			follow_wall = false;
		}
	}

	// Si estamos abrazando muro y el objetivo ya ha sido visto
	// dejamos de abrazar muro
	if (follow_wall && !follow_priority) {
		unsigned char c = position_known ? 
		mapaResultado[target_point.first][target_point.second] 
		: aux_map[target_point.first][target_point.second];
		if (c != '?') {
			follow_wall = false;
		}
	}

	// Si estamos abrazando muro y estamos en ciclo dejamos
	// de abrazarlo y permitimos paso por agua y bosque
	if (follow_wall) {
		pair<int,int> curr = {curr_state.row, curr_state.col};
		if (curr == hit_point && 
		(moved_after_hitting || init_ori == curr_state.compass)) {
			// Estamos en ciclo
			follow_wall = false;
			water_allowed = true;
			forest_allowed = true;
		}
	}

	// Si estamos en casilla de recarga seguimos si hace falta
	if (sensores.terreno[0] == 'X' && worthCharging(sensores)) {
		accion = actIDLE;
		turns_without_charging = 0;
	} else if (follow_wall) {
		// Si todavía abrazo muro y no hemos terminado ni estamos en ciclo
		pair<int,int> curr = {curr_state.row, curr_state.col};
		Action bestAction = optimalMove(target_point);
		// Si estoy en el punto de choque y puedo ir hacia adelante,
		// voy y marco que me he movido (para detectar ciclo en el futuro)
		if (curr == hit_point && !isObstacle(frente)) {
			accion = actFORWARD;
			moved_after_hitting = true;
		} else if (leave_wall) {
			// Si me he despegado de la pared
			if (isObstacle(izda) && isObstacle(frente) && isObstacle(dcha)) {
				leave_wall = false;
				moved_after_hitting = false;
				hit_point = curr;
				leave_point = {-1,-1};		
				accion = actTURN_SR;		
			} else if (bestAction == actFORWARD) {
				accion = !isObstacle(frente) ? actFORWARD : 
				(!isObstacle(izda) ? actTURN_SL : actTURN_SR);
			} else if (bestAction == actTURN_SR) {
				accion = !isObstacle(dcha) ? actTURN_SR : 
				(!isObstacle(frente) ? actFORWARD : actTURN_SL);
			} else {
				accion = !isObstacle(izda) ? actTURN_SL : 
				(!isObstacle(frente) ? actFORWARD : actTURN_SR);
			}
		} else {
			// Calculo si me debo despegar de la pared
			// Preferible seguir adelante
			if (bestAction == actFORWARD) {
				// Puedo ir adelante
				if (!isObstacle(frente)) {
					accion = actFORWARD;
					// Considero despegarme
					if (!isObstacle(izda)) {
						leave_point = curr;
						// Nos despegamos
						if (distanceBetween(leave_point, target_point) < 
						distanceBetween(hit_point, target_point)) {
							leave_wall = true;
						} else {
							accion = actTURN_SL;
						}
					}
				} else {
					// Si no puedo ir adelante, giro a la izquierda si hay huecho,
					// si no a la derecha
					accion = !isObstacle(izda) ? 
					actTURN_SL : actTURN_SR;
				}
			} else if (bestAction == actTURN_SR) {
				// Considero despegarme
				if (!isObstacle(frente)) {
					leave_point = curr;
					if (distanceBetween(leave_point, target_point) >= 
						distanceBetween(hit_point, target_point)) {
						accion = !isObstacle(izda) ? actTURN_SL : actFORWARD;
					} else {
						leave_wall = true;
						accion = actTURN_SR;
					}
				} else {
					accion = !isObstacle(izda) ? actTURN_SL : actTURN_SR;
				}
			} else {
				accion = !isObstacle(izda) ? actTURN_SL : 
				(!isObstacle(frente) ? actFORWARD : actTURN_SR);
			}
		}

	} else if (!isObstacle(frente) && frontUnexplored() && !follow_priority) {
		// Si podemos movernos adelante y hay algo sin explorar 
		// inmediatamente nos movemos adelante
		accion = actFORWARD;
	} else if (isObstacle(izda) && isObstacle(frente) 
		&& isObstacle(dcha)) {
		// Si estamos bloqueados calculamos celda '?' más cercana
		// y la acción preferible. Si seguimos punto prioritario el más cercano
		// será ese
		pair<int,int> nearest = follow_priority ? priority_point : nearestUnexploredCell();
		Action bestAction = optimalMove(nearest);
		// Si la acción preferible no es girar hacia atrás empezamos
		// a abrazar el muro. Lo sigo a mano izquierda.
		if (bestAction != actTURN_BL && bestAction != actTURN_BR) {
			follow_wall = true;
			leave_wall = false;
			moved_after_hitting = false;
			init_ori = curr_state.compass;
			hit_point = {curr_state.row, curr_state.col};
			leave_point = {-1,-1};
			target_point = nearest;
			// Elegimos dirección de giro
			follow_right = followWallRight(target_point);
			accion = follow_right ? actTURN_SR : actTURN_SL;
		} else {
			// Si lo que queríamos era ir hacia atrás, giramos
			accion = bestAction;
		}
	} else {
		// Si no está bloqueado el camino, calculamos la celda más cercana
		// y la acción preferible. Si seguimos punto prioritario, ese será el elegido
		pair<int,int> nearest = follow_priority ? priority_point : nearestUnexploredCell();
		Action bestAction = optimalMove(nearest);
		if (last_action == actTURN_BR && isObstacle(frente) && isObstacle(izda)) {
			accion = actTURN_SL;
		} else if (last_action == actTURN_BL && isObstacle(frente) && isObstacle(dcha)) {
			accion = actTURN_SR;
		} else if (bestAction == actFORWARD) {
			// Si es seguir hacia adelante, lo intentamos y si no es 
			// posible, giramos
			accion = !isObstacle(frente) ? actFORWARD : 
			(!isObstacle(dcha) ? actTURN_SR : actTURN_SL);
		} else if (bestAction == actTURN_SR) {
			// Lo mismo para giro a la derecha
			accion = !isObstacle(dcha) ? actTURN_SR : 
			(!isObstacle(frente) ? actFORWARD : actTURN_SL);
		} else if (bestAction == actTURN_SL) {
			// Lo mismo para giro a la izquierda
			accion = !isObstacle(izda) ? actTURN_SL : 
			(!isObstacle(frente) ? actFORWARD : actTURN_SR);
		} else {
			// Si lo preferible es girar hacia atrás, lo hacemos
			accion = bestAction;
		}
	}

	// Gestión de superficie
	if (accion == actFORWARD && avoidNPC(sensores)) {
		accion = actIDLE;
	}

	// Actualizamos última acción
	last_action = accion;

	// Retornamos nuestra siguiente acción
	return accion;
}

int ComportamientoJugador::interact(Action accion, int valor){
  return false;
}

pair<int,int> ComportamientoJugador::getCoordinates(int n) {
	// Posición y orientación actual
	Orientacion ori = curr_state.compass;
	int x = curr_state.row, y = curr_state.col;
	int r = 0, c = 0, rot_r = 0, rot_c = 0;
	// Contador de celda de terreno
	int count = 0;

	// Calculo coordenadas de celda que interesa como si estuviesemos 
	// situados en el origen y mirando hacia el norte/noreste
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j <= 2*i; ++j) {
			if (count == n) {
				if (ori == norte || ori == sur || ori == este || ori == oeste) {
					r = -i;
					c = j-i;
				} else {
					r = j <= i ? -i : j-2*i;
					c = j <= i ? j : i;
				}
			}
			++count;
		}
	}

	// Rotamos
	switch (ori) {
	case norte: case noreste:
		rot_r = r;
		rot_c = c;
		break;
	case este: case sureste:
		rot_r = c;
		rot_c = -r;			
		break;
	case sur: case suroeste:
		rot_r = -r;
		rot_c = -c;			
		break;
	case oeste: case noroeste:
		rot_r = -c;
		rot_c = r;			
		break;
	}

	// Trasladamos
	rot_r += x;
	rot_c += y;
	
	return {rot_r, rot_c};
}

void ComportamientoJugador::updateMap(vector<vector<unsigned char>> & v, Sensores sens) {
	pair<int,int> point;
	for (int i = 0; i < 16; ++i) {
		point = getCoordinates(i);
		v[point.first][point.second] = sens.terreno[i];
	}
}

void ComportamientoJugador::addPosToVector(vector<pair<int,int>> & v, pair<int,int> p) {
	for (auto & el : v) {
		if (el == p) {
			return;
		}
	}
	v.push_back(p);
}

void ComportamientoJugador::transferToAnswerMap(Sensores sens) {
	int r = curr_state.row-sens.posF, c = curr_state.col-sens.posC;
	unsigned char cell;
	for (int i = 0; i < map_width; ++i) {
		for (int j = 0; j < map_length; ++j) {
			// Rellenamos mapa resultado
			if (mapaResultado[i][j] == '?') {
				mapaResultado[i][j] = aux_map[r+i][c+j];
			}
			cell = mapaResultado[i][j];
			// Añadimos boosters encontrados
			if (cell == 'D') {
				addPosToVector(shoes_locations, {i,j});
			} else if (cell == 'K') {
				addPosToVector(bikini_locations, {i,j});
			}
		}
	}
}

pair<int,int> ComportamientoJugador::nearestUnexploredCell() {
	pair<int,int> answer;
	// Dimensiones del mapa
	int rows = position_known ? map_width : 2*map_width - 1;
	int cols = position_known ? map_length : 2*map_length - 1;
	// Respuesta por defecto
	answer.first = position_known ? 0 : map_width-1;
	answer.second = position_known ? 0 : map_length-1;
	
	// Booleana que es false al final de iteración si 
	// no quedan celdas por comprobar
	bool unchecked = true;
	int level = 0, pos_r = curr_state.row, pos_c = curr_state.col;
	int lbc = 0, ubc = 0, lbr = 0, ubr = 0;
	unsigned char c;

	// Mientras quedan elementos por comprobar
	while (unchecked) {
		unchecked = false;
		++level;
		lbc = pos_c - level >= 0 ? pos_c - level : 0;
		ubc = pos_c + level < cols ? pos_c + level : cols - 1;
		lbr = pos_r - level >= 0 ? pos_r - level : 0;
		ubr = pos_r + level < rows ? pos_r + level : rows - 1;

		// Norte
		if (pos_r - level >= 0) {
			unchecked = true;
			for (int i = lbc; i <= ubc; ++i) {
				c = position_known ? mapaResultado[pos_r-level][i] : aux_map[pos_r-level][i];
				if (c == '?') {
					return {pos_r-level, i};
				}
			}
		}
		// Sur
		if (pos_r + level < rows) {
			unchecked = true;
			for (int i = lbc; i <= ubc; ++i) {
				c = position_known ? mapaResultado[pos_r+level][i] : aux_map[pos_r+level][i];
				if (c == '?') {
					return {pos_r+level, i};
				}
			}
		}
		// Este
		if (pos_c + level < cols) {
			unchecked = true;
			for (int i = lbr; i <= ubr; ++i) {
				c = position_known ? mapaResultado[i][pos_c+level] : aux_map[i][pos_c+level];
				if (c == '?') {
					return {i, pos_c+level};
				}
			}
		}
		// Oeste
		if (pos_c - level >= 0) {
			unchecked = true;
			for (int i = lbr; i <= ubr; ++i) {
				c = position_known ? mapaResultado[i][pos_c-level] : aux_map[i][pos_c-level];
				if (c == '?') {
					return {i, pos_c-level};
				}
			}
		}
	}

	return answer;
}

bool ComportamientoJugador::isObstacle(unsigned char c) {
	return c == 'P' || c == 'M' || 
	(c == 'A' && !water_allowed) || (c == 'B' && !forest_allowed);
} 

bool ComportamientoJugador::frontUnexplored() {
	// Dimensiones del mapa
	int rows = position_known ? map_width : 2*map_width - 1;
	int cols = position_known ? map_length : 2*map_length - 1;
	// Posición y orientación actual
	Orientacion ori = curr_state.compass;
	int x = curr_state.row, y = curr_state.col;
	int r = 0, c = 0, rot_r = 0, rot_c = 0;
	// Recorremos las 7 celdas delanteras
	// Trabajo con norte y noreste, después hago rotación adecuada
	for (int i = 0; i < 7; ++i) {
		// Traslación de la posición actual al origen
		if (ori == norte || ori == sur || ori == este || ori == oeste) {
			r = -4;
			c = i-3;
		} else {
			if (i <= 3) {
				r = -4;
				c = i+1;
			} else {
				r = i-7;
				c = 4;
			}
		}

		// Rotación según orientación
		switch (ori) {
		case norte: case noreste:
			rot_r = r;
			rot_c = c;
			break;
		case este: case sureste:
			rot_r = c;
			rot_c = -r;			
			break;
		case sur: case suroeste:
			rot_r = -r;
			rot_c = -c;			
			break;
		case oeste: case noroeste:
			rot_r = -c;
			rot_c = r;			
			break;
		}
		// Traslación a la posición correspondiente
		rot_r += x;
		rot_c += y;
		
		// Comprobación de que hay algo en la celda
		if (rot_r < rows && rot_r >= 0 && rot_c < cols && rot_c >= 0) {
			unsigned char carac = position_known ? mapaResultado[rot_r][rot_c] :
			aux_map[rot_r][rot_c];
			if (carac == '?') {
				return true;
			}
		}
	}
	return false;
}

Action ComportamientoJugador::optimalMove(pair<int,int> & p){
	Orientacion ori = curr_state.compass;
	// Trasladamos posición actual al origen
	int x = 0, y = 0;
	int r = p.first - curr_state.row, c = p.second - curr_state.col, sector = 0;

	// Rotamos hacia norte/noreste
	switch (ori) {
	case norte: case noreste:
		x = r;
		y = c;
		break;
	case este: case sureste:
		x = -c;
		y = r;			
		break;
	case sur: case suroeste:
		x = -r;
		y = -c;			
		break;
	case oeste: case noroeste:
		x = c;
		y = -r;			
		break;
	}

	// Divido el mapa en 8 sectores mediante líneas que pasan
	// por nuestra posición con un ángulo de 45 grados de separación
	// Calculo en qué sector se encuentra el punto p
	// 0 es norte-noreste, 1 es noreste-este, etc. en sentido de las agujas del reloj

	if (ori == norte || ori == sur || ori == este || ori == oeste) {
		if (y >= 0) {
			if (x <= 0) {
				sector = y < -x ? 0 : 1;
			} else {
				sector = y >= x ? 2 : 3;
			}
		} else {
			if (x > 0) {
				sector = y >= -x ? 4 : 5;
			} else {
				sector = y <= x ? 6 : 7;
			}
		}
	} else {
		if (y >= -x) {
			if (y >= x) {
				sector = x < 0 ? 0 : 1;
			} else {
				sector = y > 0 ? 2 : 3;
			}
		} else {
			if (y < x) {
				sector = x > 0 ? 4 : 5;
			} else {
				sector = y <= 0 ? 6 : 7;
			}
		}
	}

	// Mi acción óptima la elijo como: adelante si el sector es 7/0,
	// giro 45 grados a la derecha si 1, giro 135 grados a la derecha si
	// 2/3, giro 135 grados a la izquierda si 4/5, y 45 si 6
	if (sector == 1) {
		return actTURN_SR;
	} else if (sector == 2 || sector == 3) {
		return actTURN_BR;
	} else if (sector == 4 || sector == 5) {
		return actTURN_BL;
	} else if (sector == 6) {
		return actTURN_SL;
	}

	return actFORWARD;

	/*
	// Orientación convertida a entero
	int ori = curr_state.compass;
	// Coordenadas de p si centramos nuestra posición en el origen
	int x = p.first - curr_state.row, y = p.second - curr_state.col;
	int sector = 0;
	// Divido el mapa en 8 sectores mediante líneas que pasan
	// por nuestra posición con un ángulo de 45 grados de separación
	// Calculo en qué sector se encuentra el punto p
	// 0 es norte-noreste, 1 es noreste-este, etc. en sentido de las agujas del reloj

	if (y >= 0) {
		if (x <= 0) {
			sector = y < -x ? 0 : 1;
		} else {
			sector = y >= x ? 2 : 3;
		}
	} else {
		if (x > 0) {
			sector = y >= -x ? 4 : 5;
		} else {
			sector = y <= x ? 6 : 7;
		}
	}

	// El agente puede estar orientado en 8 direcciones distintas
	// Si giro al agente hacia el norte y giro el sector, la acción 
	// escogida sería la misma
	sector = (sector - ori + 8)%8;

	// Ahora tenemos al agente "mirando" al norte
	// Mi acción óptima la elijo como: adelante si el sector es 7/0,
	// giro 45 grados a la derecha si 1, giro 135 grados a la derecha si
	// 2/3, giro 135 grados a la izquierda si 4/5, y 45 si 6
	if (sector == 1) {
		return actTURN_SR;
	} else if (sector == 2 || sector == 3) {
		return actTURN_BR;
	} else if (sector == 4 || sector == 5) {
		return actTURN_BL;
	} else if (sector == 6) {
		return actTURN_SL;
	}

	return actFORWARD;
	*/
} 

int ComportamientoJugador::distanceBetween(pair<int,int> & p1, pair<int,int> & p2) {
	return (p1.first-p2.first)*(p1.first-p2.first)
	+(p1.second-p2.second)*(p1.second-p2.second);
}

bool ComportamientoJugador::worthCharging(Sensores sens) {
	return sens.vida * 3.5 > sens.bateria && sens.bateria < 5000;
} 

bool ComportamientoJugador::prioritySpotted(Sensores sens, pair<int,int> & p) {
	unsigned char c;
	bool worth_charging = turns_without_charging > 300 && worthCharging(sens);
	for (int i = 1; i < 16; ++i) {
		c = sens.terreno[i];
		if (!position_known && c == 'G' || !shoes_on && c == 'D'
		|| !bikini_on && c == 'K' || worth_charging && c == 'X') {
			p = getCoordinates(i);
			return true;
		}
	}
	return false;
}

bool ComportamientoJugador::avoidNPC(Sensores sens) {

	if (sens.superficie[2] == 'l' || sens.superficie[5] == 'l' || sens.superficie[6] == 'l' 
	|| sens.superficie[7] == 'l' || sens.superficie[2] == 'a' ) {
		return true;
	}
	return false;
}

bool ComportamientoJugador::followWallRight(pair<int,int> p) {
	return true;
}


