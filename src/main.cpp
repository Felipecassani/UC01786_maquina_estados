#include <Arduino.h>
#include <Bounce2.h>

/*
 * UC01786 - Maquina de estados generica
 *
 * Entradas: START, STOP, emergencia, sensor de condicao e RESET.
 * Saidas:   atuador simulado, LED verde, LED amarelo e LED vermelho.
 *
 * As entradas sao ligadas entre o pino e GND e utilizam INPUT_PULLUP.
 * Cada entrada passa por debounce (Bounce2) para filtrar ruido mecanico
 * do contacto e para detectar a transicao do botao (evita accionar a
 * mesma ordem varias vezes enquanto o botao permanece premido).
 */

constexpr byte pinoStart = 2;
constexpr byte pinoStop = 3;
constexpr byte pinoEmergencia = 4;
constexpr byte pinoSensor = 5;
constexpr byte pinoReset = 6;

constexpr byte pinoActuador = 8;
constexpr byte pinoLedVerde = 9;
constexpr byte pinoLedAmarelo = 10;
constexpr byte pinoLedVermelho = 11;

constexpr unsigned long tempoArranque = 3000;
constexpr unsigned long tempoParagem = 2000;
constexpr unsigned long tempoDebounce = 25; // ms

Bounce debounceStart;
Bounce debounceStop;
Bounce debounceEmergencia;
Bounce debounceSensor;
Bounce debounceReset;

enum EstadoSistema
{
    DESLIGADO,
    VERIFICACAO,
    ARRANQUE,
    FUNCIONAMENTO,
    PARAGEM,
    AVARIA
};

EstadoSistema estadoAtual = DESLIGADO;
unsigned long inicioEstado = 0;

const char *nomeEstado(EstadoSistema estado)
{
    switch (estado)
    {
    case DESLIGADO:
        return "DESLIGADO";
    case VERIFICACAO:
        return "VERIFICACAO";
    case ARRANQUE:
        return "ARRANQUE";
    case FUNCIONAMENTO:
        return "FUNCIONAMENTO";
    case PARAGEM:
        return "PARAGEM";
    case AVARIA:
        return "AVARIA";
    default:
        return "DESCONHECIDO";
    }
}

void mudarEstado(EstadoSistema novoEstado)
{
    estadoAtual = novoEstado;
    inicioEstado = millis();
    Serial.print("Novo estado: ");
    Serial.println(nomeEstado(estadoAtual));
}

void desligarSaidas()
{
    digitalWrite(pinoActuador, LOW);
    digitalWrite(pinoLedVerde, LOW);
    digitalWrite(pinoLedAmarelo, LOW);
    digitalWrite(pinoLedVermelho, LOW);
}

void configurarDebounce(Bounce &debounce, byte pino)
{
    debounce.attach(pino, INPUT_PULLUP);
    debounce.interval(tempoDebounce);
}

void setup()
{
    Serial.begin(9600);

    configurarDebounce(debounceStart, pinoStart);
    configurarDebounce(debounceStop, pinoStop);
    configurarDebounce(debounceEmergencia, pinoEmergencia);
    configurarDebounce(debounceSensor, pinoSensor);
    configurarDebounce(debounceReset, pinoReset);

    pinMode(pinoActuador, OUTPUT);
    pinMode(pinoLedVerde, OUTPUT);
    pinMode(pinoLedAmarelo, OUTPUT);
    pinMode(pinoLedVermelho, OUTPUT);

    desligarSaidas();
    Serial.println("Sistema iniciado - estado DESLIGADO");
}

void loop()
{
    debounceStart.update();
    debounceStop.update();
    debounceEmergencia.update();
    debounceSensor.update();
    debounceReset.update();

    // START, STOP e RESET disparam so na transicao (fell = acabou de ser
    // premido), para nao repetir a ordem enquanto o botao fica premido.
    const bool startPremido = debounceStart.fell();
    const bool stopPremido = debounceStop.fell();
    const bool resetPremido = debounceReset.fell();

    // Emergencia e sensor sao lidos ao nivel: a condicao mantem-se
    // enquanto o interruptor permanecer accionado.
    const bool emergenciaAtiva = debounceEmergencia.read() == LOW;
    const bool sensorOk = debounceSensor.read() == LOW;

    // A emergencia tem prioridade sobre a sequencia normal.
    if (emergenciaAtiva && estadoAtual != AVARIA)
    {
        mudarEstado(AVARIA);
    }

    switch (estadoAtual)
    {
    case DESLIGADO:
        desligarSaidas();
        if (startPremido)
        {
            mudarEstado(VERIFICACAO);
        }
        break;

    case VERIFICACAO:
        digitalWrite(pinoLedAmarelo, HIGH);
        if (!sensorOk)
        {
            Serial.println("Avaria: condicao de seguranca nao validada.");
            mudarEstado(AVARIA);
        }
        else
        {
            mudarEstado(ARRANQUE);
        }
        break;

    case ARRANQUE:
        digitalWrite(pinoActuador, HIGH);
        digitalWrite(pinoLedAmarelo, HIGH);
        if (stopPremido)
        {
            mudarEstado(PARAGEM);
        }
        else if (!sensorOk)
        {
            Serial.println("Avaria durante o arranque.");
            mudarEstado(AVARIA);
        }
        else if (millis() - inicioEstado >= tempoArranque)
        {
            mudarEstado(FUNCIONAMENTO);
        }
        break;

    case FUNCIONAMENTO:
        digitalWrite(pinoActuador, HIGH);
        digitalWrite(pinoLedVerde, HIGH);
        digitalWrite(pinoLedAmarelo, LOW);
        if (stopPremido)
        {
            mudarEstado(PARAGEM);
        }
        else if (!sensorOk)
        {
            Serial.println("Avaria: perda da condicao de seguranca.");
            mudarEstado(AVARIA);
        }
        break;

    case PARAGEM:
        digitalWrite(pinoActuador, LOW);
        digitalWrite(pinoLedVerde, LOW);
        digitalWrite(pinoLedAmarelo, HIGH);
        if (millis() - inicioEstado >= tempoParagem)
        {
            mudarEstado(DESLIGADO);
        }
        break;

    case AVARIA:
        digitalWrite(pinoActuador, LOW);
        digitalWrite(pinoLedVerde, LOW);
        digitalWrite(pinoLedAmarelo, LOW);
        digitalWrite(pinoLedVermelho, HIGH);
        if (resetPremido && !emergenciaAtiva && sensorOk)
        {
            mudarEstado(DESLIGADO);
        }
        break;
    }
}
