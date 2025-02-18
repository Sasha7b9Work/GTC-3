// 2022/05/06 11:30:06 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once


struct Key
{
    enum E
    {
        _1,
        _2,
        Count
    };

    E value;

    Key(E v) : value(v) {}

    bool IsPressed() const;
};


extern Key key1;
extern Key key2;


namespace Keyboard
{
    void Init();

    void Update();
}
