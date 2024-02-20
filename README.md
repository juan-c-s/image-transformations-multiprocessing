 ## Transformaciones de imágenes

## Compilación
```bash
g++-13 --std=c++17 image_transform.cpp -o A
```

## Rotar
``./A r [ARCHIVO_LECTURA] [ARCHIVO_SALIDA]``
```bash
./A r boat.bmp boatRotado.bmp
```

## Sumar
./A s [ARCHIVO_LECTURA] [ARCHIVO_SALIDA] [ARCHIVO_LECTURA2]``
```bash
./A s boat.bmp fotoSumada.bmp Cascada.bmp
```

## Trasladar
Entrar vector de traslado (desplazamiento en X y Y)\
./A t [ARCHIVO_LECTURA] [ARCHIVO_SALIDA] [X] [Y]
```bash
./A t boat.bmp boatTrasladado.bmp 500 200
```