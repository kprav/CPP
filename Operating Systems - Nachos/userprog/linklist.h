#include "copyright.h"

class LinkListElement
{
 public:
  LinkListElement(void *itemPtr, int id);
  void *item;
  int index;
}


class LinkList
{
 public:
LinkList
 private:
  LinkListElement *first; 
  LinkListElement *last;
}
