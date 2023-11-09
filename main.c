/* Standard includes. */
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"


// Comunica??o via fila de mensagens
QueueHandle_t fila_mensagem;
QueueHandle_t mensagem_motor;

// Declara??o de uma vari?vel mutex
SemaphoreHandle_t mutex_motor;

void lerInput();
void alterarDisplay();
void ligarMotor();
void desligarMotor();

struct elevador {
    int cur_andar;
    int dest_andar;
};

unsigned int countSetBits(unsigned int n)
{
    unsigned int count = 0;
    while (n) {
        count += n & 1;
        n >>= 1;
    }
    return count;
};

/*
 * Create the demo tasks then start the scheduler.
 */
int main(void) {

    // Cria a fila de mensagens
    fila_mensagem = xQueueCreate(1, sizeof(struct elevador));
    mensagem_motor = xQueueCreate(1, sizeof(int));
    
    // Cria??o de uma vari?vel mutex
    mutex_motor = xSemaphoreCreateMutex();

    /* Create the test tasks defined within this file. */
    xTaskCreate(lerInput, "lerInput", 128, NULL, 2, NULL);
    xTaskCreate(alterarDisplay, "alterarDisplay", 128, NULL, 4, NULL);
    xTaskCreate(ligarMotor, "ligarMotor", 128, NULL, 5, NULL);
    xTaskCreate(desligarMotor, "desligarMotor", 128, NULL, 5, NULL);

    /* Finally start the scheduler. */
    vTaskStartScheduler();
        
    LATD = 0;
    /* Will only reach here if there is insufficient heap available to start
    the scheduler. */
    return 0;
}

void lerInput()
{
    TRISEbits.TRISE0 = 0;
    TRISEbits.TRISE1 = 0;
    TRISEbits.TRISE2 = 0;
    TRISEbits.TRISE3 = 0;
    TRISBbits.TRISB0 = 1;
    TRISBbits.TRISB1 = 1;
    TRISBbits.TRISB2 = 1;
    TRISBbits.TRISB3 = 1;
    CNPU1bits.CN2PUE = 1;
    CNPU1bits.CN3PUE = 1;
    CNPU1bits.CN4PUE = 1;
    CNPU1bits.CN5PUE = 1;
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD1 = 0;
    LATE = 0b0001;

    struct elevador e;
    while (1) {
        e.dest_andar = PORTB;
        e.cur_andar = LATE;
        // Checagem se o andar atual é diferente do andar de destino e verifica se só há um destino pedido ao mesmo tempo
        if(e.dest_andar != e.cur_andar && countSetBits(PORTB) == 1) {
            xQueueSend(fila_mensagem, &e, portMAX_DELAY); // envia as informações atuais do elevador
        }
        else {
            xSemaphoreGive(mutex_motor); // Caso o andar for igual ou tem múltiplos botões apertados, o motor é desligado
        }
        
        vTaskDelay(50);     
    }
}

// Se encarrega de alterar o display do andar atual e envia a mensagem de direção ao gerenciador do motor
void alterarDisplay()
{
    struct elevador e;
    int direcao = 2;
    while (1) {
        xQueueReceive(fila_mensagem, &e, portMAX_DELAY); // recebe a mensagem da tarefa de leitura com as informações do elevador
        if (e.cur_andar < e.dest_andar) {
            e.cur_andar = e.cur_andar << 1; // shifta o valor do andar atual para a esquerda
            LATE = e.cur_andar; // seta os indicadores de andar igual ao andar atual
            direcao = 1; // seta a direção do elevador para descer
            xQueueSend(mensagem_motor, &direcao, portMAX_DELAY); // envia a informação para subir o elevador        

        } else if(e.cur_andar > e.dest_andar) {
            e.cur_andar = e.cur_andar >> 1; // shifta o valor do andar atual para a direita
            LATE = e.cur_andar; // seta os indicadores de andar igual ao andar atual
            direcao = 0; // seta a direção do elevador para descer
            xQueueSend(mensagem_motor, &direcao, portMAX_DELAY); // envia a informação para descer o elevador           
        }
        
        vTaskDelay(50);
    }
}

// Se encarrega de ligar o motor para a direção especificada pela tarefa alterarDisplay
void ligarMotor()
{
    int direcao;
    while(1){
        xQueueReceive(mensagem_motor, &direcao, portMAX_DELAY); // recebe a mensagem da tarefa changeDisplay
        
        // baseado na mensagem recebida, liga o motor para a cima (direita) ou para baixo (esquerda)
        if(direcao == 0) {
            LATDbits.LATD0 = 0;
            LATDbits.LATD1 = 1;
        }
        else {
            LATDbits.LATD0 = 1;
            LATDbits.LATD1 = 0;
        }
        
        vTaskDelay(50);
    }
}

// Tarefa encarregada de desligar o motor
void desligarMotor()
{
    while(1){
        xSemaphoreTake(mutex_motor, portMAX_DELAY); // pega o mutex do motor
        PORTD = 0; // desliga o motor
        vTaskDelay(50);
    }
}