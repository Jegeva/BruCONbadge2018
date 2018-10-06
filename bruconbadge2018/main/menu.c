#include "menu.h"
#include "nokialcd.h"
#include "expat.h"


uint8_t    menu_dirty    = 1;
uint8_t    currselected  = 0;
menuItem * currMenuItem = NULL ;
char     * statusbartext =NULL;
int        statusbartextallocedssz = 0;

menuItp menuhead;
menuItp schedhead_d1;
menuItp schedhead_d2;
menuItp schedhead_d3;

menuItem fakeplaceholder;


void fixparent(menuItp p, menuItp h)
{
    while(h != NULL){
        h->parent = p;
        h=h->next;
    }
}

char skeleton_inited = 0;

void sched03(void* arg){
    menuItp mitp;
    currMenuItem->child = schedhead_d1;
    fixparent(currMenuItem,currMenuItem->child);
    currMenuItem->funcp = NULL;
    manage_click_menu(PIN_BR,0);
};

void sched04(void* arg){
    currMenuItem->child = schedhead_d2;
    fixparent(currMenuItem,currMenuItem->child);
    currMenuItem->funcp = NULL;
    manage_click_menu(PIN_BR,0);

};

void sched05(void* arg){
    currMenuItem->child = schedhead_d3;
    fixparent(currMenuItem,currMenuItem->child);
    currMenuItem->funcp = NULL;
    manage_click_menu(PIN_BR,0);
};


void MapWest(void* arg){
    fakeplaceholder.parent = currMenuItem;
    currMenuItem = &fakeplaceholder;
    MapWestL(arg);
    skeleton_inited = 0;
}

void MapNovo(void* arg){
    fakeplaceholder.parent = currMenuItem;
    currMenuItem = &fakeplaceholder;
    MapNovoL(arg);
    skeleton_inited = 0;
}

void VivaLaVodka(void * arg)
{
  // for you richard...
  TaskHandle_t Tasktemp;
  fakeplaceholder.parent = currMenuItem;
  currMenuItem = &fakeplaceholder;
  uint32_t score = VivaLaVodkaL(arg);
  xTaskCreate( &  send_alc_reading  , "SendAlc" , 4096, &score , 5| portPRIVILEGE_BIT , &Tasktemp);
  skeleton_inited = 0;
  while(ispostingAlc){
    vTaskDelay(200);
  };
}


void dispSchedItem(void* arg){
    lcd_clearB12(B12_WHITE);
    char * wrkstr = calloc(128,sizeof(char));
    char * tmp;

    memset(wrkstr,0,128);
    sprintf(wrkstr,"Loc: %s",currMenuItem->schedloc);
    lcd_setStr(wrkstr,2,0,BLACK,B12_WHITE,0,1);

    switch(currMenuItem->schedtype){
    case 'W':
        sprintf(wrkstr,"Type: Workshop");
        break;
    case 'T':
        sprintf(wrkstr,"Type: Talk");
    break;

    case 'L':
        sprintf(wrkstr,"Type: General");
        break;

    case 'V':
        sprintf(wrkstr,"Type: Village");

    break;

    case 'S':
        sprintf(wrkstr,"Type: Sp3ci4l");
    tmp = "";
    break;
    default:
        printf("unknown type %c\n",currMenuItem->schedtype);
    }

    lcd_setStr(wrkstr,32,0,BLACK,B12_WHITE,0,1);
    lcd_setRect(30,0, 30,131, 1, 0);
    lcd_setRect(47,0, 94,131, 1, B12_RED);
    lcd_setStr(currMenuItem->menuText+12,48,0,B12_WHITE,B12_RED,0,1);
    lcd_setRect(46,0, 46,131, 1, 0);
    if(currMenuItem->schedspk != NULL)
    lcd_setStr(currMenuItem->schedspk,96,0,BLACK,B12_WHITE,0,1);
    lcd_setRect(94,0, 94,131, 1, 0);
    free(wrkstr);
    fakeplaceholder.parent = currMenuItem;
    currMenuItem = &fakeplaceholder;
}


#define AUTHORIZED_FUNC_NBR 6

void * menuAuthorisedFunc[AUTHORIZED_FUNC_NBR] = {
    sched03,
    sched04,
    sched05,
    MapWest,
    MapNovo,
    VivaLaVodka
};

char * menuAuthorisedFuncName[AUTHORIZED_FUNC_NBR] = {
    "sched03",
    "sched04",
    "sched05",
    "MapWest",
    "MapNovo",
    "VivaLaVodka"
};


void manage_click_menu(uint32_t value,uint32_t level ){

    int oldrank=currselected;
    menuItem * oldmenuitem = currMenuItem;

    if(currMenuItem == NULL){
        printf("CURR NULL!\n");

        return;

    }
    //  if(value == PIN_NCHARGING){
    //     printf("clkc: ncharg %d \n",level);
    // }

    if(level==0){
//        printf("clkc: %d \n",value);
        if(value != PIN_NCHARGING){

            switchbacklight(1);
            last_click=xTaskGetTickCount();
        }

        switch(value){
        case PIN_BL:
            if(currMenuItem->parent != NULL){
                currMenuItem = currMenuItem->parent;
                currselected = currMenuItem->rank;
                menu_dirty    = 1;
            }
            break;
        case PIN_BR:
            if(currMenuItem->funcp != NULL){
                menu_dirty    = 0;
                currMenuItem->funcp(NULL);
            } else {

                if(currMenuItem->child != NULL){
                    currMenuItem = currMenuItem->child;
                    currselected = currMenuItem->rank;
                    menu_dirty    = 1;
                }
            }
            break;
        case PIN_BU:
            if(currMenuItem->prev != NULL){
                currMenuItem = currMenuItem->prev;
                currselected = currMenuItem->rank;
                menu_dirty    = 1;
            }
            break;
        case PIN_BD:
            if(currMenuItem->next != NULL){
                currMenuItem = currMenuItem->next;
                currselected = currMenuItem->rank;
                menu_dirty    = 1;
            }
            break;
        case PIN_BA:
            if(currMenuItem->parent != NULL){
                currMenuItem = currMenuItem->parent;
                currselected = currMenuItem->rank;
                menu_dirty    = 1;
            }
            break;
        case PIN_BB:

            if(currMenuItem->funcp != NULL){
                menu_dirty    = 0;
                currMenuItem->funcp(NULL);
            } else {

                if(currMenuItem->child != NULL){
                    currMenuItem = currMenuItem->child;
                    currselected = currMenuItem->rank;
                    menu_dirty    = 1;
                }
            }
            break;
        }
    }
    if(menu_dirty && ( (value ==PIN_BU ) || ( value == PIN_BD)           ) ){

        if((currselected != 0) && (((currselected%7)==0)&& (value == PIN_BD) ) ){
            return;
        }

        if((value ==PIN_BU ) && (oldrank == 7) )
            return;


        lcd_setRect((((oldrank%7)+1)*16)+1,0, (((oldrank%7)+2)*16)-1,131, 1, B12_WHITE);
        lcd_setStr(oldmenuitem->menuText,(((oldrank%7)+1)*16)+2,0,BLACK,B12_WHITE,0,0);
        lcd_setRect((((currselected%7)+1)*16)+1,0, (((currselected%7)+2)*16)-1,131, 1, B12_RED);
        lcd_setStr(currMenuItem->menuText,(((currselected%7)+1)*16)+2,0,B12_WHITE,B12_RED,0,0);
        lcd_setRect(((oldrank%7)+1)*16,0, ((oldrank%7)+1)*16,131, 1, 0);
        lcd_setRect(((currselected%7)+1)*16,0,((currselected%7)+1)*16,131, 1, 0);
        menu_dirty=0;

    }
}
void * menuAuthorisedFuncByName(char * funcname){
    int i;
    for(i=0;i<AUTHORIZED_FUNC_NBR;i++){
        if(strcmp(funcname,menuAuthorisedFuncName[i]) == 0){
            return menuAuthorisedFunc[i];
        }
    }
    return NULL;
}

void parserMenuTree(menuItp acurrmenuItem,const cJSON* subtree, int depth){
    cJSON *elptr = NULL;
    const cJSON *thisel = NULL;
    menuItp currmenuItem=acurrmenuItem;
    cJSON_ArrayForEach(elptr, subtree){

        thisel = cJSON_GetObjectItemCaseSensitive(elptr, "menutitle");
        if (cJSON_IsString(thisel) && (thisel->valuestring != NULL))
            currmenuItem->menuText= thisel->valuestring;

        thisel = cJSON_GetObjectItemCaseSensitive(elptr, "rank");
        if (cJSON_IsNumber(thisel))
            currmenuItem->rank    = thisel->valueint;

        thisel = cJSON_GetObjectItemCaseSensitive(elptr, "function");
        if (cJSON_IsString(thisel) && (thisel->valuestring != NULL)){
            currmenuItem->funcp = menuAuthorisedFuncByName(thisel->valuestring);
            if(currmenuItem->funcp == NULL){
                printf("unauthorised json function: %s\n",thisel->valuestring);
            }
        }
        thisel = cJSON_GetObjectItemCaseSensitive(elptr, "submenus");
        if(cJSON_IsArray(thisel)){
            //      printf("\nrec\n");
            currmenuItem->child = (menuItp)calloc(1,sizeof(menuItem));
            currmenuItem->child->parent = currmenuItem;
            parserMenuTree(currmenuItem->child,thisel , depth+1);
        }
        if(elptr->next != NULL){
            currmenuItem->next = (menuItp)calloc(1,sizeof(menuItem));
            currmenuItem->next->prev = currmenuItem;
            currmenuItem->next->parent = acurrmenuItem->parent;
            currmenuItem = currmenuItem->next;
        }


    }
//    printf("\nD;%d\n",depth);

}




void parserSchedTreeT(menuItp acurrmenuItem,const cJSON* subtree, int depth){
    cJSON *elptr = NULL;
    const cJSON *thisel = NULL;
    int rank = 0;

    menuItp currmenuItem=acurrmenuItem;

    cJSON_ArrayForEach(elptr, subtree){
        thisel = cJSON_GetObjectItemCaseSensitive(elptr, "title");
        if (cJSON_IsString(thisel) && (thisel->valuestring != NULL)){
            currmenuItem->menuText= thisel->valuestring;
        }
        thisel = cJSON_GetObjectItemCaseSensitive(elptr, "evt");
        if (cJSON_IsString(thisel) && (thisel->valuestring != NULL)){
            currmenuItem->schedtype= *thisel->valuestring;
        }
        thisel = cJSON_GetObjectItemCaseSensitive(elptr, "loc");
        if (cJSON_IsString(thisel) && (thisel->valuestring != NULL)){
            currmenuItem->schedloc = thisel->valuestring;
        }
        thisel = cJSON_GetObjectItemCaseSensitive(elptr, "descr");
        if (cJSON_IsString(thisel) && (thisel->valuestring != NULL)){
            currmenuItem->schedentry = thisel->valuestring;
        }
        thisel = cJSON_GetObjectItemCaseSensitive(elptr, "spk");
        if (cJSON_IsString(thisel) && (thisel->valuestring != NULL)){
            currmenuItem->schedspk = thisel->valuestring;
        }
        currmenuItem->rank = rank++;
        currmenuItem->funcp = dispSchedItem;
        if(elptr->next != NULL){
            currmenuItem->next = (menuItp)calloc(1,sizeof(menuItem));
            currmenuItem->next->prev = currmenuItem;
            currmenuItem = currmenuItem->next;
        }

    }

}

void parserSchedTree(const cJSON* tree){
  schedhead_d1 = calloc(1,sizeof(struct menuIt));
  schedhead_d2 = calloc(1,sizeof(struct menuIt));
  schedhead_d3 = calloc(1,sizeof(struct menuIt));

  parserSchedTreeT(schedhead_d1,tree->child,0);
  parserSchedTreeT(schedhead_d2,tree->child->next,0);
  parserSchedTreeT(schedhead_d3,tree->child->next->next,0);

}
cJSON * parserMenuJson(char* jsonstring,uint8_t issched){
    cJSON *menu_json = cJSON_Parse(jsonstring);
    int depth = 0;
    if (menu_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL){
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
    }
    if(issched){
        parserSchedTree(menu_json);
    } else {
        menuhead = (menuItp)calloc(1,sizeof(menuItem));
        parserMenuTree(menuhead,menu_json,depth);
    }

    return menu_json;
}



void set_status_bar_text(char * b){
  int blen = strlen(b);
  if(blen > statusbartextallocedssz){
    statusbartext = realloc(statusbartext,blen+1);
    statusbartextallocedssz = blen+1;    
  }
  strncpy(statusbartext,b,blen);
  *(statusbartext+blen+1)=0;  
}

char used[7];

void display_skeleton()
{
    int i;

    menuItem * dispmenuitem = currMenuItem;
    if(! skeleton_inited){
        lcd_clearB12(B12_WHITE);
        lcd_setRect((currselected+1)*16,0, (currselected+2)*16,131, 1, B12_RED);
        skeleton_inited = 1;
    }
    lcd_setRect(0,0, 16,131, 1, 0);

    while((dispmenuitem->prev != NULL) && (dispmenuitem->rank %7 ) )
        dispmenuitem=dispmenuitem->prev;
    i=0;
    while( (i < 7) && (dispmenuitem != NULL) ){
        if(used[i]){

            lcd_setRect(((i+1)*16)+1,0, ((i+2)*16)-1,131, 1, (i== (currselected%7) )?B12_RED:B12_WHITE);
        }

        used[i]=1;
        lcd_setStr(dispmenuitem->menuText,((i+1)*16)+2,0,(i==(currselected%7))?B12_WHITE:BLACK,(i==(currselected%7))?B12_RED:B12_WHITE,0,0);
        i++;
        dispmenuitem=dispmenuitem->next;
    }
    while(( i < 7) && used[i])
    {
        used[i]=0;
        lcd_setRect(((i+1)*16)+1,0, ((i+2)*16)-1,131, 1, B12_WHITE);
        i++;
    }

    for(i=1;i<8;i++)
        lcd_setRect(i*16,0, i*16,131, 1, 0);



}

gpio_num_t pins_buttons[7] = { 
PIN_BL,
PIN_BR,
PIN_BU,
PIN_BD,
PIN_BA,
PIN_BB,
PIN_NCHARGING
};

char pins_levels[7] ;
TickType_t pins_debounce[7];
TickType_t last_click;


static xQueueHandle gpio_evt_queue = NULL;

static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    int pinrank;
    char level;
    char lastlevel;
    // switchbacklight(1);
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            level = gpio_get_level(io_num);
            switch(io_num){
            case PIN_BL:
                pinrank=0;
                break;
            case PIN_BR:
                pinrank=1;
                break;
            case PIN_BU:
                pinrank=2;
                break;
            case PIN_BD:
                pinrank=3;
                break;
            case PIN_BA:
                pinrank=4;
                break;
            case PIN_BB:
                pinrank=5;
                break;
            case PIN_NCHARGING:
                pinrank=6;
                break;
            default:
                return;
                break;
            }
            lastlevel = pins_levels[pinrank];
            pins_levels[pinrank] = level;
            if( (xTaskGetTickCount()-pins_debounce[pinrank]) > 0){
                pins_debounce[pinrank]=xTaskGetTickCount();
            } else {
                lastlevel=0;
            }


            if( (lastlevel) && (level==0))
                manage_click(io_num,level);
        }
    }
}



static void IRAM_ATTR handler_gpio_buttons(void *args) {
  
  uint32_t gpio_num = (uint32_t) args;
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}


void print_menu(menuItp aitem, int depth){
    menuItp item = aitem;
    int i=depth;
    do {
        for(i=0;i<depth;i++)
            putchar(' ');
        printf("%s\n",item->menuText);
        if(item->schedentry != NULL){
            printf(" :%s\n",item->schedentry);
        }

        if(item->child)
            print_menu(item->child, depth+1);
        item=item->next;
    } while(item);

}

char    menu_inited=0;
extern const uint8_t menu_json_start[] asm("_binary_menu_json_start");
extern const uint8_t sched_json_start[] asm("_binary_schedule_sched_filtered_json_start");

char * netsched = NULL;


void display_menu(uint8_t clicked_gui_value)
{
    int i;

    if(! menu_inited){
        menu_inited=1;

        gpio_config_t io_conf;
        io_conf.intr_type = GPIO_PIN_INTR_ANYEDGE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = 0; // all are physically pulled up
        io_conf.pin_bit_mask =
            (1ULL<<PIN_BR )|
            (1ULL<<PIN_BU )|
            (1ULL<<PIN_BD )|
            (1ULL<<PIN_BA )|
            (1ULL<<PIN_BB )|
            (1ULL<<PIN_BL )
            ;//|            (1ULL<<PIN_NCHARGING );

        //create a queue to handle gpio event from isr
        gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
        //start gpio task
        xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);


        parserMenuJson(menu_json_start,0);
        //   print_menu(menuhead,0);
        currMenuItem = menuhead;
        currselected = currMenuItem->rank;


        printf("getting sched\n");


        if(netsched != NULL){
            parserMenuJson(netsched,1);
            free(netsched);

        } else {
            printf("fetching sched broke, using embedded schedule\n");
            parserMenuJson(sched_json_start,1);
        }
        switchbacklight(1);

        /*    print_menu(schedhead_d1,0);
        print_menu(schedhead_d2,0);
        print_menu(schedhead_d3,0);*/

        gpio_config(&io_conf);

        gpio_install_isr_service(0);
        gpio_isr_handler_add(PIN_BL	         , handler_gpio_buttons, (void*)PIN_BL  );
        gpio_isr_handler_add(PIN_BR	         , handler_gpio_buttons, (void*)PIN_BR  );
        gpio_isr_handler_add(PIN_BU	         , handler_gpio_buttons, (void*)PIN_BU  );
        gpio_isr_handler_add(PIN_BD	         , handler_gpio_buttons, (void*)PIN_BD  );
        gpio_isr_handler_add(PIN_BB	         , handler_gpio_buttons, (void*)PIN_BB  );
        gpio_isr_handler_add(PIN_BA	         , handler_gpio_buttons, (void*)PIN_BA  );
        //     gpio_isr_handler_add(PIN_NCHARGING	 , handler_gpio_buttons, (void*)PIN_NCHARGING);
        printf("gpio handlers installed\n");
        for(i=0;i<7;i++){
            pins_levels[i]=1;
        }

    }
  

    if(menu_dirty){
        display_skeleton();
        menu_dirty=0;
    }


}

