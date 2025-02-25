#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"

// Bibliotecas para a matriz LED
#include <math.h>
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/bootrom.h"
#include "hardware/gpio.h"

// Arquivo .pio
#include "ws2818b.pio.h"

#define LED_VERDE 11
#define LED_VERMELHO 12
#define LED_AUX 13
#define BOTAO_PONTO 5
#define BOTAO_TRACO 6

#define LED_COUNT 25
#define LED_PIN 7

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;


// Inicialização do display OLED
ssd1306_t oled;

// Estrutura para armazenar as letras e seus códigos Morse
typedef struct {
    char letra;
    char* codigo_morse;
} MorseCode;

// Tabela de Código Morse (algumas letras para exemplo)
MorseCode tabela_morse[] = {
    {'A', ".-"},
    {'B', "-..."},
    {'C', "-.-."},
    {'D', "-.."},
    {'E', "."},
    {'F', "..-."},
    {'G', "--."},
    {'H', "...."},
    {'I', ".."},
    {'J', ".---"},
    {'K', "-.-"}
};

// Função para obter o código Morse de uma letra
char* obter_codigo_morse(char letra) {
    for (int i = 0; i < sizeof(tabela_morse) / sizeof(MorseCode); i++) {
        if (tabela_morse[i].letra == letra) {
            return tabela_morse[i].codigo_morse;
        }
    }
    return NULL;
}



// Captura a entrada do usuário em Morse
void receber_entrada_usuario(char* entrada_usuario) {
    int index = 0;
    absolute_time_t tempo_inicio = get_absolute_time();

    while (index < 5) { // Evita buffer overflow
        if (!gpio_get(BOTAO_PONTO)) {
            sleep_ms(400);
            entrada_usuario[index++] = '.';
            printf(".");
            tempo_inicio = get_absolute_time();
            sleep_ms(300);
        }
        if (!gpio_get(BOTAO_TRACO)) {
            sleep_ms(400);
            entrada_usuario[index++] = '-';
            printf("-");
            tempo_inicio = get_absolute_time();
            sleep_ms(300);
        }

        // Sai do loop se o usuário ficar 2 segundos sem pressionar nada
        if (absolute_time_diff_us(tempo_inicio, get_absolute_time()) > 2000000) {
            break;
        }
    }
    entrada_usuario[index] = '\0'; // Finaliza string
}


volatile int contador_ponto = 0;
volatile int contador_traco = 0;
volatile int nine = 0;

// Variáveis para debouncing
volatile uint64_t ultimo_tempo_ponto = 0;
volatile uint64_t ultimo_tempo_traco = 0;
const uint64_t debounce_time_us = 100000; // 100 ms para debouncing

// Função de interrupção
void gpio_callback(uint gpio, uint32_t events) {
    uint64_t agora = time_us_64(); // Obtém o tempo atual em microssegundos

    if (gpio == BOTAO_PONTO && (events & GPIO_IRQ_EDGE_FALL)) {
        if ((agora - ultimo_tempo_ponto) > debounce_time_us) {
            nine  ++; // Alternar para a letra A
           
        }
        ultimo_tempo_ponto = agora; // Atualiza o tempo da última interrupção
    }

    if (gpio == BOTAO_TRACO && (events & GPIO_IRQ_EDGE_FALL)) {
        if ((agora - ultimo_tempo_traco) > debounce_time_us) {
            nine  --; // Alternar para a letra B
           
        }
        ultimo_tempo_traco = agora; // Atualiza o tempo da última interrupção
    }
}


int main() {
  


    stdio_init_all();

    // Configurar GPIO dos botões
    gpio_init(BOTAO_PONTO);
    gpio_set_dir(BOTAO_PONTO, GPIO_IN);
    gpio_pull_up(BOTAO_PONTO);

    gpio_init(BOTAO_TRACO);
    gpio_set_dir(BOTAO_TRACO, GPIO_IN);
    gpio_pull_up(BOTAO_TRACO);

    // Configurar GPIO dos LEDs
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_put(LED_VERDE, 0);

    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);
    gpio_put(LED_VERMELHO, 0);

    gpio_init(LED_AUX);
    gpio_set_dir(LED_AUX, GPIO_OUT);
    gpio_put(LED_AUX, 0);

    // Configurar I2C para o OLED
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);


   // Registrar a interrupção no BOTAO_PONTO (com callback)
   gpio_set_irq_enabled_with_callback(BOTAO_PONTO, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

   // Registrar a interrupção no BOTAO_TRACO (sem callback, pois já foi registrado no BOTAO_PONTO)
   gpio_set_irq_enabled(BOTAO_TRACO, GPIO_IRQ_EDGE_FALL, true);


    ssd1306_init();

       // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
       struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    restart:



    char letra_atual = 'A';  // Pode ser aleatório depois
    char* codigo_correto = obter_codigo_morse(letra_atual);

 

  


        
        while (1) {
            // Exibir a letra no OLED



            while(nine == 0){
            char *text[] = {"  Letra A  ", "  em Morse: .- "};
            int y = 0;
            for (uint i = 0; i < count_of(text); i++) {
                ssd1306_draw_string(ssd, 5, y, text[i]);
                y += 8;
            }
            render_on_display(ssd, &frame_area);
    
            // Resetar os contadores

    
            // Esperar a sequência correta
           
    
            // Indicar acerto
            gpio_put(LED_VERDE, 1);
            sleep_ms(500);
            gpio_put(LED_VERDE, 0);
            sleep_ms(250);
            gpio_put(LED_VERDE, 1);
            sleep_ms(1000);
            gpio_put(LED_VERDE, 0);
            sleep_ms(250);

    
        
            sleep_ms(1700);

        

            }




            while(nine == 1){
                char *text[] = {"  Letra B  ", "  em Morse: -... "};
                int y = 0;
                for (uint i = 0; i < count_of(text); i++) {
                    ssd1306_draw_string(ssd, 5, y, text[i]);
                    y += 8;
                }
                render_on_display(ssd, &frame_area);
        
                // Resetar os contadores
    
        
                // Esperar a sequência correta
               
        
                // Indicar acerto
                gpio_put(LED_VERMELHO, 1);
                sleep_ms(1000);
                gpio_put(LED_VERMELHO, 0);
                sleep_ms(250);
                gpio_put(LED_VERMELHO, 1);
                sleep_ms(500);
                gpio_put(LED_VERMELHO, 0);
                sleep_ms(250);
                gpio_put(LED_VERMELHO, 1);
                sleep_ms(500);
                gpio_put(LED_VERMELHO, 0);
                sleep_ms(250);
                gpio_put(LED_VERMELHO, 1);
                sleep_ms(500);
                gpio_put(LED_VERMELHO, 0);
                sleep_ms(250);
    
        
            
                sleep_ms(1700);
    
            
    
                }




                while(nine == 2){
                    char *text[] = {"  Letra C  ", "  em Morse: -.-. "};
                    int y = 0;
                    for (uint i = 0; i < count_of(text); i++) {
                        ssd1306_draw_string(ssd, 5, y, text[i]);
                        y += 8;
                    }
                    render_on_display(ssd, &frame_area);
            
                    // Resetar os contadores
        
            
                    // Esperar a sequência correta
                   
            
                    // Indicar acerto
                    gpio_put(LED_AUX, 1);
                    sleep_ms(1000);
                    gpio_put(LED_AUX, 0);
                    sleep_ms(250);
                    gpio_put(LED_AUX, 1);
                    sleep_ms(500);
                    gpio_put(LED_AUX, 0);
                    sleep_ms(250);
                    gpio_put(LED_AUX, 1);
                    sleep_ms(1000);
                    gpio_put(LED_AUX, 0);
                    sleep_ms(250);
                    gpio_put(LED_AUX, 1);
                    sleep_ms(500);
                    gpio_put(LED_AUX, 0);
                    sleep_ms(250);
        
            
                
                    sleep_ms(1700);
        
                
        
                    }





                    while(nine == 3){
                        char *text[] = {"  Letra D  ", "  em Morse: -.. "};
                        int y = 0;
                        for (uint i = 0; i < count_of(text); i++) {
                            ssd1306_draw_string(ssd, 5, y, text[i]);
                            y += 8;
                        }
                        render_on_display(ssd, &frame_area);
                
                        // Resetar os contadores
            
                
                        // Esperar a sequência correta
                       
                
                        // Indicar acerto
                        gpio_put(LED_VERDE, 1);
                        sleep_ms(1000);
                        gpio_put(LED_VERDE, 0);
                        sleep_ms(250);
                        gpio_put(LED_VERDE, 1);
                        sleep_ms(500);
                        gpio_put(LED_VERDE, 0);
                        sleep_ms(250);
                        gpio_put(LED_VERDE, 1);
                        sleep_ms(500);
                        gpio_put(LED_VERDE, 0);
                        sleep_ms(250);
            
                
                    
                        sleep_ms(1700);
            
                    
            
                        }


                        while(nine == 4){
                            char *text[] = {"  Letra E  ", "  em Morse: . "};
                            int y = 0;
                            for (uint i = 0; i < count_of(text); i++) {
                                ssd1306_draw_string(ssd, 5, y, text[i]);
                                y += 8;
                            }
                            render_on_display(ssd, &frame_area);
                    
                            // Resetar os contadores
                
                    
                            // Esperar a sequência correta
                           
                    
                            // Indicar acerto
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                       
                
                    
                        
                            sleep_ms(1700);
                
                        
                
                            }

                
                            while(nine == 5){
                                char *text[] = {"  Letra F  ", "  em Morse: ..-. "};
                                int y = 0;
                                for (uint i = 0; i < count_of(text); i++) {
                                    ssd1306_draw_string(ssd, 5, y, text[i]);
                                    y += 8;
                                }
                                render_on_display(ssd, &frame_area);
                        
                                // Resetar os contadores
                    
                        
                                // Esperar a sequência correta
                               
                        
                                // Indicar acerto
                                gpio_put(LED_AUX, 1);
                                sleep_ms(500);
                                gpio_put(LED_AUX, 0);
                                sleep_ms(250);
                                gpio_put(LED_AUX, 1);
                                sleep_ms(500);
                                gpio_put(LED_AUX, 0);
                                sleep_ms(250);
                                gpio_put(LED_AUX, 1);
                                sleep_ms(1000);
                                gpio_put(LED_AUX, 0);
                                sleep_ms(250);
                                gpio_put(LED_AUX, 1);
                                sleep_ms(500);
                                gpio_put(LED_AUX, 0);
                                sleep_ms(250);
                           
                    
                        
                            
                                sleep_ms(1700);
                    
                            
                    
                                }


            
                                while(nine == 6){
                                    char *text[] = {"  Letra G  ", "  em Morse: --. "};
                                    int y = 0;
                                    for (uint i = 0; i < count_of(text); i++) {
                                        ssd1306_draw_string(ssd, 5, y, text[i]);
                                        y += 8;
                                    }
                                    render_on_display(ssd, &frame_area);
                            
                                    // Resetar os contadores
                        
                            
                                    // Esperar a sequência correta
                                   
                            
                                    // Indicar acerto
                                    gpio_put(LED_VERDE, 1);
                                    sleep_ms(1000);
                                    gpio_put(LED_VERDE, 0);
                                    sleep_ms(250);
                                    gpio_put(LED_VERDE, 1);
                                    sleep_ms(1000);
                                    gpio_put(LED_VERDE, 0);
                                    sleep_ms(250);
                                    gpio_put(LED_VERDE, 1);
                                    sleep_ms(500);
                                    gpio_put(LED_VERDE, 0);
                                    sleep_ms(250);
                               
                        
                            
                                
                                    sleep_ms(1700);
                        
                                
                        
                                    }



                                    while(nine == 7){
                                        char *text[] = {"  Letra H  ", "  em Morse: .... "};
                                        int y = 0;
                                        for (uint i = 0; i < count_of(text); i++) {
                                            ssd1306_draw_string(ssd, 5, y, text[i]);
                                            y += 8;
                                        }
                                        render_on_display(ssd, &frame_area);
                                
                                        // Resetar os contadores
                            
                                
                                        // Esperar a sequência correta
                                       
                                
                                        // Indicar acerto
                                        gpio_put(LED_VERMELHO, 1);
                                        sleep_ms(500);
                                        gpio_put(LED_VERMELHO, 0);
                                        sleep_ms(250);
                                        gpio_put(LED_VERMELHO, 1);
                                        sleep_ms(500);
                                        gpio_put(LED_VERMELHO, 0);
                                        sleep_ms(250);
                                        gpio_put(LED_VERMELHO, 1);
                                        sleep_ms(500);
                                        gpio_put(LED_VERMELHO, 0);
                                        sleep_ms(250);
                                        gpio_put(LED_VERMELHO, 1);
                                        sleep_ms(500);
                                        gpio_put(LED_VERMELHO, 0);
                                        sleep_ms(250);
                                        
                                   
                            
                                
                                    
                                        sleep_ms(1700);
                            
                                    
                            
                                        }

                                        while(nine == 8){
                                            char *text[] = {"  Letra I  ", "  em Morse: .. "};
                                            int y = 0;
                                            for (uint i = 0; i < count_of(text); i++) {
                                                ssd1306_draw_string(ssd, 5, y, text[i]);
                                                y += 8;
                                            }
                                            render_on_display(ssd, &frame_area);
                                    
                                            // Resetar os contadores
                                
                                    
                                            // Esperar a sequência correta
                                           
                                    
                                            // Indicar acerto
                                            gpio_put(LED_AUX, 1);
                                            sleep_ms(500);
                                            gpio_put(LED_AUX, 0);
                                            sleep_ms(250);
                                            gpio_put(LED_AUX, 1);
                                            sleep_ms(500);
                                            gpio_put(LED_AUX, 0);
                                            sleep_ms(250);
                                          
                                
                                       
                                
                                    
                                        
                                            sleep_ms(1700);
                                
                                        
                                
                                            }

                        while(nine == 9){
                        char *text[] = {"  Letra J  ", "  em Morse: .--- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                             sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                 
                                    
                            sleep_ms(1700);
                                                }


                        while(nine == 10){
                        char *text[] = {"  Letra K  ", "  em Morse: -.- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                 
                                    
                            sleep_ms(1700);
                                                }
                   while(nine == 11){
                        char *text[] = {"  Letra L  ", "  em Morse: .-.. "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(1000);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            
                 
                                    
                            sleep_ms(1700);
                                                }

                   while(nine == 12){
                        char *text[] = {"  Letra M  ", "  em Morse: -- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                          
                 
                                    
                            sleep_ms(1700);
                                                }
                             while(nine == 13){
                        char *text[] = {"  Letra N  ", "  em Morse: -. "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                           
                 
                                    
                            sleep_ms(1700);
                                                }



                            while(nine == 14){
                        char *text[] = {"  Letra O  ", "  em Morse: --- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_AUX, 1);
                            sleep_ms(1000);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(1000);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(1000);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                   
                           
                 
                                    
                            sleep_ms(1700);
                                                }


    
                            while(nine == 15){
                        char *text[] = {"  Letra P  ", "  em Morse: .--. "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                           
                           
                   
                           
                 
                                    
                            sleep_ms(1700);
                                                }


                              while(nine == 16){
                        char *text[] = {"  Letra Q  ", "  em Morse: --.- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                             
                            sleep_ms(1700);
                                                }


                              while(nine == 17){
                        char *text[] = {"  Letra R  ", "  em Morse: .-. "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(1000);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                        
                        
                             
                            sleep_ms(1700);
                                                }


                while(nine == 18){
                        char *text[] = {"  Letra S  ", "  em Morse: ... "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            
                        
                        
                             
                            sleep_ms(1700);
                                                }


                                                
    
                        while(nine == 19){
                        char *text[] = {"  Letra T  ", "  em Morse: - "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                          
                             
                            sleep_ms(1700);
                                                }

        
                        while(nine == 20){
                        char *text[] = {"  Letra U  ", "  em Morse: ..- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(1000);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                          
                             
                            sleep_ms(1700);
                                                }

        
    
                        while(nine == 21){
                        char *text[] = {"  Letra V  ", "  em Morse: ...- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);

                          
                             
                            sleep_ms(1700);
                                                }


                        while(nine == 22){
                        char *text[] = {"  Letra W ", "  em Morse: .-- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            

                          
                             
                            sleep_ms(1700);
                                                }

                        while(nine == 23){
                        char *text[] = {"  Letra X ", "  em Morse: -..- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_AUX, 1);
                            sleep_ms(1000);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(1000);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                           
                           

                          
                             
                            sleep_ms(1700);
                                                }


                        while(nine == 24){
                        char *text[] = {"  Letra Y ", "  em Morse: -.-- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                        
                             
                            sleep_ms(1700);
                                                }



                 while(nine == 25){
                        char *text[] = {"  Letra Z ", "  em Morse: --.. "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                           
                           
                             
                            sleep_ms(1700);
                                                }

                    
                          while(nine >= 26){
                     nine = 0;
                     sleep_ms(1400);
                                                }


                        while(nine == -1){
                        char *text[] = {"  S.O.S  ", "em Morse  "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            sleep_ms(1000);

                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            sleep_ms(1000);

                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            sleep_ms(1000);

                            
                           
                           
                             
                            sleep_ms(1700);
                                                }


                         while(nine == -2){
                        char *text[] = {"  Numero 1 ", "  em Morse: .---- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                           
                           
                             
                            sleep_ms(1700);
                                                }




                            while(nine == -3){
                        char *text[] = {"  Numero 2 ", "  em Morse: .---- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(1000);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(1000);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(1000);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                           
                           
                             
                            sleep_ms(1700);
                                                }


                         while(nine == -4){
                        char *text[] = {"  Numero 3 ", "  em Morse: .---- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            
                             
                            sleep_ms(1700);
                                                }




                    while(nine == -5){
                        char *text[] = {"  Numero 4 ", "  em Morse: .---- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                           
                             
                            sleep_ms(1700);
                                                }


                      while(nine == -6){
                        char *text[] = {"  Numero 5 ", "  em Morse: .---- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            
                            
                           
                             
                            sleep_ms(1700);
                                                }


                     while(nine == -7){
                        char *text[] = {"  Numero 6 ", "  em Morse: .---- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                           
                           
                             
                            sleep_ms(1700);
                                                }

            
                     while(nine == -8){
                        char *text[] = {"  Numero 7 ", "  em Morse: .---- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                           
                             
                            sleep_ms(1700);
                                                }

                  while(nine == -9){
                        char *text[] = {"  Numero 8 ", "  em Morse: .---- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_AUX, 1);
                            sleep_ms(1000);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(1000);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(1000);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                            gpio_put(LED_AUX, 1);
                            sleep_ms(500);
                            gpio_put(LED_AUX, 0);
                            sleep_ms(250);
                           
                           
                           
                           
                             
                            sleep_ms(1700);
                                                }


                 while(nine == -10){
                        char *text[] = {"  Numero 9 ", "  em Morse: .---- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERDE, 1);
                            sleep_ms(500);
                            gpio_put(LED_VERDE, 0);
                            sleep_ms(250);
                            
                
                            sleep_ms(1700);
                                                }

               while(nine == -11){
                        char *text[] = {"  Numero 0 ", "  em Morse: .---- "};
                           int y = 0;
                           for (uint i = 0; i < count_of(text); i++) {
                           ssd1306_draw_string(ssd, 5, y, text[i]);
                             y += 8;
                            }
                           render_on_display(ssd, &frame_area);
                                        
 
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                            gpio_put(LED_VERMELHO, 1);
                            sleep_ms(1000);
                            gpio_put(LED_VERMELHO, 0);
                            sleep_ms(250);
                           
                            
                
                            sleep_ms(1700);
                                                }
                

              while(nine <= -12){
                     nine = 0;
                     sleep_ms(1400);
                                                }

    
    
    
        }
    
    
    



    return 0;
}