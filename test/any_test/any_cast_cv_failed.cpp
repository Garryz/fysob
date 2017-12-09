// Copyright 2006 Alexander Nasonov.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <engine/common/any.h>

int main()
{
    engine::any const a;
    engine::any_cast<int&>(a);
}

