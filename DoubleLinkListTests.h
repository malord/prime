// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DOUBLELINKLISTTESTS_H
#define PRIME_DOUBLELINKLISTTESTS_H

#include "DoubleLinkList.h"

namespace Prime {

namespace DoubleLinkListTestsPrivate {
    class Item : public NonAtomicRefCounted<Item> {
    public:
        DoubleLink<Item> renderLink;
        DoubleLink<Item> updateLink;
    };

    template <typename LinkList>
    static Item* AddItem(Item* item, LinkList* list1)
    {
        list1->push_back(item);
        return item;
    }

    template <typename DoubleLinkList>
    static void DoubleLinkListTest2()
    {
        DoubleLinkList updateList(&Item::updateLink);
        DoubleLinkList renderList(&Item::renderLink);

        AddItem(PassRef(new Item), &updateList);
        AddItem(PassRef(new Item), &updateList);
        AddItem(AddItem(PassRef(new Item), &updateList), &renderList);
        AddItem(AddItem(PassRef(new Item), &updateList), &renderList);
        AddItem(PassRef(new Item), &renderList);
        AddItem(PassRef(new Item), &renderList);

        for (Item* item = renderList.getFirst(); item; item = renderList.getNext(item)) {
            PRIME_TEST(renderList.contains(item));
        }

        for (Item* item = updateList.getFirst(); item; item = updateList.getNext(item)) {
            PRIME_TEST(updateList.contains(item));
        }

        PRIME_TEST(!renderList.contains(updateList.getFirst()));
        PRIME_TEST(!updateList.contains(renderList.getLast()));
        PRIME_TEST(renderList.isAfter(renderList.getLast(), renderList.getFirst()));
        PRIME_TEST(updateList.isAfter(updateList.getLast(), updateList.getFirst()));

        updateList.clear();
        renderList.clear();
    }
}

inline void DoubleLinkListTests()
{
    using namespace DoubleLinkListTestsPrivate;

    DoubleLinkListTest2<DoubleLinkList<Item, RefCountingLinkListElementManager<Item>>>();

    // I was toying with the idea of having a circular linked list type but it seems to trade fewer branches when
    // adding/removing elements against complicating the list traversal logic:
    // if (! list.empty()) {
    //     Item* item = list.getFirst();
    //     do {
    //         // Do something with the item
    //     } while (item = list.getNext(item), item != list.getFirst());
    // }
    //
    // vs.
    //
    // for (Item* item = list.getFirst(); item; item = list.getNext(item))
    //     // Do something with the item
}
}

#endif
