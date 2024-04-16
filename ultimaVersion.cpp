#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

struct Aspirante {
  std::string id;
  int x, y;
  std::string localidad;
};

struct Edificio {
  std::string id;
  int x, y;
  std::string localidad;
};

struct Salon {
  std::string id;
  std::string edificio;
  int piso;
  int max;
  int opt;
  int min;
  int capacidad_actual = 0; // Añadir un contador de capacidad actual
};

std::tuple<std::vector<Aspirante>, std::vector<Edificio>, std::vector<Salon>>
cargar_datos(const std::string &archivo) {
  std::ifstream file(archivo);
  std::string line;
  std::vector<Aspirante> lista_aspirantes;
  std::vector<Edificio> lista_edificios;
  std::vector<Salon> lista_salones;

  // Leer número de aspirantes
  int N;
  std::getline(file, line);
  N = std::stoi(line);

  for (int i = 0; i < N; ++i) {
    std::getline(file, line);
    std::istringstream iss(line);
    Aspirante aspirante;
    std::getline(iss, aspirante.id, ',');
    iss >> aspirante.x;
    iss.ignore(); // Ignorar la coma
    iss >> aspirante.y;
    iss.ignore(); // Ignorar la coma
    std::getline(iss, aspirante.localidad, ',');
    lista_aspirantes.push_back(aspirante);
  }

  // Leer número de edificios
  int M;
  std::getline(file, line);
  M = std::stoi(line);

  for (int i = 0; i < M; ++i) {
    std::getline(file, line);
    std::istringstream iss(line);
    Edificio edificio;
    std::getline(iss, edificio.id, ',');
    iss >> edificio.x;
    iss.ignore(); // Ignorar la coma
    iss >> edificio.y;
    iss.ignore(); // Ignorar la coma
    std::getline(iss, edificio.localidad, ',');
    lista_edificios.push_back(edificio);
  }

  // Leer número de salones
  int K;
  std::getline(file, line);
  K = std::stoi(line);

  for (int i = 0; i < K; ++i) {
    std::getline(file, line);
    std::istringstream iss(line);
    Salon salon;
    std::getline(iss, salon.id, ',');
    std::getline(iss, salon.edificio, ',');
    iss >> salon.piso;
    iss.ignore(); // Ignorar la coma
    iss >> salon.min;
    iss.ignore(); // Ignorar la coma
    iss >> salon.opt;
    iss.ignore(); // Ignorar la coma
    iss >> salon.max;
    lista_salones.push_back(salon);
  }

  return {lista_aspirantes, lista_edificios, lista_salones};
}

// Estructura para el nodo del KD-tree
struct NodoKD {
  Edificio edificio;           // El edificio en este nodo
  NodoKD *izquierda = nullptr; // Hijos izquierdo y derecho del nodo
  NodoKD *derecha = nullptr;
};

// Función recursiva para insertar un edificio en el KD-tree
NodoKD *insertar(NodoKD *raiz, const Edificio &edificio, int profundidad = 0) {
  // Si el nodo actual está vacío, coloca el edificio aquí
  if (!raiz) {
    raiz = new NodoKD();
    raiz->edificio = edificio;
    return raiz;
  }

  // Calcula el eje actual (0 para x, 1 para y)
  int eje = profundidad % 2;

  // Compara el edificio actual con el edificio en el nodo
  // Dependiendo del eje, inserta en el subárbol izquierdo o derecho
  if ((eje == 0 && edificio.x < raiz->edificio.x) ||
      (eje == 1 && edificio.y < raiz->edificio.y)) {
    raiz->izquierda = insertar(raiz->izquierda, edificio, profundidad + 1);
  } else {
    raiz->derecha = insertar(raiz->derecha, edificio, profundidad + 1);
  }

  return raiz;
}

// Función para construir un KD-tree con la lista de edificios
NodoKD *construir_KDtree(const std::vector<Edificio> &lista_edificios) {
  NodoKD *raiz = nullptr;
  for (const auto &edificio : lista_edificios) {
    raiz = insertar(raiz, edificio);
  }
  return raiz;
}
double distancia(int x1, int y1, int x2, int y2) {
  return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
}

Edificio vecino_mas_cercano(const Aspirante &aspirante, NodoKD *raiz,
                            int profundidad = 0,
                            std::set<std::string> excluidos = {}) {
  if (!raiz) {
    return {};
  }

  int eje = profundidad % 2;
  NodoKD *siguiente = nullptr;
  NodoKD *otro = nullptr;

  if ((eje == 0 && aspirante.x < raiz->edificio.x) ||
      (eje == 1 && aspirante.y < raiz->edificio.y)) {
    siguiente = raiz->izquierda;
    otro = raiz->derecha;
  } else {
    siguiente = raiz->derecha;
    otro = raiz->izquierda;
  }

  Edificio edificio_cercano =
      vecino_mas_cercano(aspirante, siguiente, profundidad + 1, excluidos);

  if (excluidos.find(raiz->edificio.id) == excluidos.end() &&
      (edificio_cercano.id.empty() ||
       distancia(aspirante.x, aspirante.y, raiz->edificio.x, raiz->edificio.y) <
           distancia(aspirante.x, aspirante.y, edificio_cercano.x,
                     edificio_cercano.y))) {
    edificio_cercano = raiz->edificio;
  }

  if (otro) {
    double d = (eje == 0) ? abs(aspirante.x - raiz->edificio.x)
                          : abs(aspirante.y - raiz->edificio.y);
    Edificio edificio_otro =
        vecino_mas_cercano(aspirante, otro, profundidad + 1, excluidos);
    if (edificio_otro.id.empty() ||
        distancia(aspirante.x, aspirante.y, edificio_otro.x, edificio_otro.y) <
            d) {
      edificio_cercano = edificio_otro;
    }
  }

  return edificio_cercano;
}

std::vector<Salon *> obtener_salones_de(const Edificio &edificio,
                                        std::vector<Salon> &lista_salones) {
  std::vector<Salon *> salones_del_edificio;
  for (Salon &salon : lista_salones) {
    if (salon.edificio == edificio.id) {
      salones_del_edificio.push_back(&salon);
    }
  }
  return salones_del_edificio;
}

Salon *buscar_salon_disponible(std::vector<Salon *> &salones,
                               const std::string &hasta) {
  for (Salon *salon : salones) {
    if (hasta == "optima" && salon->capacidad_actual < salon->opt) {
      return salon;
    } else if (hasta == "maxima" && salon->capacidad_actual < salon->max) {
      return salon;
    }
  }
  return nullptr;
}

void asignar_aspirante_a_edificio(const Aspirante &aspirante, NodoKD *kdtree,
                                  std::vector<Salon> &lista_salones) {
  std::set<std::string> edificios_considerados;

  while (true) {
    Edificio edificio_cercano =
        vecino_mas_cercano(aspirante, kdtree, 0, edificios_considerados);
    if (edificio_cercano.id.empty()) {
      break;
    }

    auto salones_del_edificio =
        obtener_salones_de(edificio_cercano, lista_salones);

    Salon *salon_disponible =
        buscar_salon_disponible(salones_del_edificio, "optima");
    if (!salon_disponible) {
      salon_disponible =
          buscar_salon_disponible(salones_del_edificio, "maxima");
    }

    if (salon_disponible) {
      salon_disponible->capacidad_actual++;
      std::cout << "Aspirante " << aspirante.id << " asignado al salón "
                << salon_disponible->id << " del edificio "
                << salon_disponible->edificio << "." << std::endl;

      return;
    }

    edificios_considerados.insert(edificio_cercano.id);
  }

  std::cout << "No hay suficientes salones. Alquilar otro edificio."
            << std::endl;
}

void asignar_aspirantes_con_KDtree(
    NodoKD *kdtree, const std::vector<Aspirante> &lista_aspirantes,
    std::vector<Salon> &lista_salones) {
  for (const Aspirante &aspirante : lista_aspirantes) {
    asignar_aspirante_a_edificio(aspirante, kdtree, lista_salones);
  }
}

void liberar_KDtree(NodoKD *nodo) {
  if (!nodo)
    return; // Si el nodo es nulo, simplemente regresa

  liberar_KDtree(nodo->izquierda); // Libera el subárbol izquierdo
  liberar_KDtree(nodo->derecha);   // Libera el subárbol derecho

  delete nodo; // Elimina el nodo actual
}
int main() {
  // 1. Cargar datos del archivo
  auto [lista_aspirantes, lista_edificios, lista_salones] =
      cargar_datos("prueba1.txt");

  // 2. Construir el KD-tree con la lista de edificios
  NodoKD *kdtree = construir_KDtree(lista_edificios);

  // 3. Asignar aspirantes usando el KD-tree
  asignar_aspirantes_con_KDtree(kdtree, lista_aspirantes, lista_salones);
  for (const auto &salon : lista_salones) {
    std::cout << "Salón " << salon.id << " del edificio " << salon.edificio
              << " tiene ahora " << salon.capacidad_actual << " aspirantes."
              << std::endl;
  }

  // 4. Liberar memoria del KD-tree
  liberar_KDtree(kdtree);

  return 0;
}

