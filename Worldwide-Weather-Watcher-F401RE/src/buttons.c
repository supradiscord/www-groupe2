#include "buttons.h"
#include "board_pins.h"

/*
 * Gestion des boutons par INTERRUPTION (EXTI).
 * - Bouton VERT  = BUTTON1 = D8 = PA9  (ligne EXTI 9)
 * - Bouton ROUGE = BUTTON2 = D9 = PC7  (ligne EXTI 7)
 * Les boutons sont actifs a l'etat bas (0 = appuye).
 *
 * L'interruption memorise l'instant de l'appui ; la boucle principale
 * (Buttons_Update) detecte quand l'appui dure 5 s et genere un evenement.
 */

#define DEBOUNCE_MS 30U

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    volatile bool pressed;
    volatile uint32_t press_tick;
    volatile uint32_t last_edge;
    bool long_fired;
} Button;

static Button green = { BUTTON1_PORT, BUTTON1_PIN, false, 0, 0, false };
static Button red   = { BUTTON2_PORT, BUTTON2_PIN, false, 0, 0, false };

static volatile ButtonEvent pending_event = BTN_EVT_NONE;

static bool read_pressed(const Button *b)
{
    return HAL_GPIO_ReadPin(b->port, b->pin) == GPIO_PIN_RESET;
}

void Buttons_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Mode = GPIO_MODE_IT_RISING_FALLING;   /* interruption sur appui ET relache */
    gpio.Pull = GPIO_PULLUP;

    gpio.Pin = green.pin;
    HAL_GPIO_Init(green.port, &gpio);
    gpio.Pin = red.pin;
    HAL_GPIO_Init(red.port, &gpio);

    /* Etat initial (utile pour detecter le bouton rouge appuye au demarrage) */
    green.pressed = read_pressed(&green);
    red.pressed   = read_pressed(&red);
    green.press_tick = red.press_tick = HAL_GetTick();

    /* Les lignes EXTI 7 et 9 partagent le meme vecteur : EXTI9_5 */
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

/* Routine d'interruption appelee par le materiel */
void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(BUTTON1_PIN);
    HAL_GPIO_EXTI_IRQHandler(BUTTON2_PIN);
}

static void on_edge(Button *b)
{
    uint32_t now = HAL_GetTick();
    if (now - b->last_edge < DEBOUNCE_MS) return;   /* anti-rebond */
    b->last_edge = now;

    bool p = read_pressed(b);
    if (p && !b->pressed)
    {
        b->press_tick = now;     /* debut de l'appui */
        b->long_fired = false;
    }
    b->pressed = p;
}

/* Callback HAL appele depuis HAL_GPIO_EXTI_IRQHandler */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == green.pin) on_edge(&green);
    else if (GPIO_Pin == red.pin) on_edge(&red);
}

static void check_long(Button *b, ButtonEvent evt)
{
    if (b->pressed && !b->long_fired &&
        (HAL_GetTick() - b->press_tick) >= BUTTON_LONG_PRESS_MS)
    {
        b->long_fired = true;
        pending_event = evt;
    }
}

void Buttons_Update(void)
{
    check_long(&red, BTN_EVT_RED_LONG);
    check_long(&green, BTN_EVT_GREEN_LONG);
}

ButtonEvent Buttons_GetEvent(void)
{
    ButtonEvent e = pending_event;
    pending_event = BTN_EVT_NONE;
    return e;
}

bool Buttons_IsRedPressed(void)   { return read_pressed(&red); }
bool Buttons_IsGreenPressed(void) { return read_pressed(&green); }
