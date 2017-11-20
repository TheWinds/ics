#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
    // wp_pool[i].expression="";
    wp_pool[i].value=0;
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

// create watchpoint from wp poll 
WP* new_wp(){
  WP* wp;
  if(free_!=NULL){
    wp=free_;
    free_=free_->next;
    wp->next=NULL;    
    WP* p=head;
    if(p==NULL){
      head=wp;
    }else{
      while(p){
        if(p->next==NULL){
          p->next=wp;
          break;
        }
        p=p->next;
      }
    }
  }else{
    wp=NULL;
  }
  return wp;
}

void free_wp(WP* wp){
  if(wp==NULL) return;
  // remove from head
  WP* p=head;
  if(p==wp){
    head=head->next;
    return;
  }
  while(p!=NULL){
    if(p->next==wp){
      WP* should_free=p->next;
      p->next=should_free->next;
      //add p->next to free_
      if(free_==NULL){
        free_=should_free;
        free_->next=NULL;
      }else{
        should_free->next=free_;
        free_=should_free;
      }
    }
    p=p->next;
  }
}

int add_wp(char *expression,char* err){
  WP* wp=new_wp();
  if(wp==NULL){
    err="too many watchpoints maxnum is 32";
    return 0;
  }
  bool expr_pass=false;
  uint32_t val=expr(expression,&expr_pass);  
  if(!expr_pass){
    err="expression check failed";
    return 0;    
  }
  strcpy(wp->expression,expression);
  wp->value=val;
  // Log("add_wp: %p => %s",wp->expression,wp->expression);
  return wp->NO;
}

bool del_wp(int no){
  WP* p=head;
  bool find=false;
  while(p!=NULL){
    if(p->NO==no){
      free_wp(p);
      find=true;
      break;
    }
    p=p->next;
  }
  return find;
}

void show_watchpoints(){
  WP* p=head;
  printf("NO\tWhat\n");
  while(p!=NULL){
    printf("%d\t%s\n",p->NO,p->expression);
    // Log("show_watchpoints: %p => %s",
    // p->expression,p->expression);
    p=p->next;
  }
}

// bool check_watchpoints(){

// }

