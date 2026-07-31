# Máquina de Estados Genérica — Arduino

Exercício de máquina de estados para Arduino, com entradas de START, STOP,
emergência, sensor de condição e reset. Todas as entradas passam por
debounce ([Bounce2](https://github.com/thomasfredericks/Bounce2)) para
filtrar ruído mecânico dos contactos.

## Hardware
- Arduino (INPUT_PULLUP nos botões, ligados entre o pino e GND)
- Saídas: atuador simulado, LED verde, LED amarelo, LED vermelho

## Objetivo
Prática de máquinas de estado em C++ para Arduino — parte da disciplina UC01786.
