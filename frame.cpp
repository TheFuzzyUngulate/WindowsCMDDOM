#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include "../include/frame.hpp"
#include "../include/cmdio.hpp"

void CMDBox::shift(int x, int y) {
    if (posx + x >= 0 && posy + y >= 0) {
        posx += x; posy += y;
    } else throw std::runtime_error("out of range");
}

void CMDFrame::shift(int x, int y) {
    if (posx + x >= 0 && posy + y >= 0) {
        CMDBox::shift(x, y);
        for (auto c_set = children; c_set != NULL; c_set = c_set->next) {
            for (auto box : c_set->members) box->setPosition(box->boxPosition);
        }
    }
}

void CMDGrid::shift(int x, int y) {
    if (posx + x >= 0 && posy + y >= 0) {
        CMDBox::shift(x, y);

        for (int ty = 0; ty < rows.count; ++ty) {
            for (int tx = 0; tx < columns.count; ++tx)
            {
                auto box = data[ty][tx];
                box->posx += x;
                box->posy += y;
            }
        }
    }
}

void CMDBox::setBorder(char ch)
{
    borders.topleft = ch;
    borders.topright = ch;
    borders.botmleft = ch;
    borders.botmright = ch;

    borders.topbody = ch;
    borders.botmbody = ch;
    borders.leftbody = ch;
    borders.rightbody = ch;
}

void CMDGrid::setBorder(char ch)
{
    tableborderch = ch;
}

void CMDBox::setZIndex(int zindex)
{
    if (static_cast<CMDFrame*>(parent) != nullptr)
    {
        CMDFrame* tp = (CMDFrame*)parent;
        tp->removeChild(this);
        tp->addChild(this, zindex);
    }
}

void CMDBox::setPosition(uint32_t x, uint32_t y, bool isRelative)
{
    posx = (isRelative && parent != NULL) ? parent->posx + x : x;
    posy = (isRelative && parent != NULL) ? parent->posy + y : y;
}

void CMDFrame::setPosition(uint32_t x, uint32_t y, bool isRelative)
{
    CMDBox::setPosition(x, y, isRelative);
    for (auto c_set = children; c_set != NULL; c_set = c_set->next) {
        for (auto box : c_set->members) box->setPosition(box->boxPosition);
    }
}

void CMDGrid::setPosition(uint32_t x, uint32_t y, bool isRelative)
{
    int oldx = parent->posx;
    int oldy = parent->posy;

    CMDBox::setPosition(x, y, isRelative);

    int offsetx = parent->posx - oldx;
    int offsety = parent->posy - oldy;

    for (int ty = 0; ty < rows.count; ++ty) {
        for (int tx = 0; tx < columns.count; ++tx)
        {
            auto box = data[ty][tx];
            box->shift(offsetx, offsety);
        }
    }
}

void CMDBox::setPosition(TextPosition pos)
{
    if (parent != NULL)
    {
        // set position
        boxPosition = pos;

        int xmin = parent->posx;
        int xmax = xmin + parent->width - 1;
        int ymin = parent->posy;
        int ymax = ymin + parent->height - 1;

        if (parent->bordered)
        {
            xmin += 1;
            xmax -= 1;
            ymin += 1;
            ymax -= 1;
        }

        int xmid = ((xmax - xmin) / 2) + xmin + ((xmax - xmin) % 2);
        int ymid = ((ymax - ymin) / 2) + ymin + ((ymax - ymin) % 2);

        int tposy, tposx;
        switch (pos)
        {
            case TRUE_CENTER:
                tposy = ymid - height/2;
                tposx = xmid - width/2;
                break;

            case CENTER_LEFT:
                tposy = ymid - height/2;
                tposx = xmin;
                break;

            case CENTER_RIGHT:
                tposy = ymid - height/2;
                tposx = xmax - width + 1;
                break;

            case TOP_CENTER:
                tposy = ymin;
                tposx = xmid - width/2;
                break;

            case TOP_LEFT:
                tposy = ymin;
                tposx = xmin;
                break;

            case TOP_RIGHT:
                tposy = ymin;
                tposx = xmax - width + 1;
                break;
                
            case BOTTOM_CENTER:
                tposy = ymax - height + 1;
                tposx = xmid - width/2;
                break;
                
            case BOTTOM_LEFT:
                tposy = ymax - height + 1;
                tposx = xmin;
                break;
                
            case BOTTOM_RIGHT:
                tposy = ymax - height + 1;
                tposx = xmax - width + 1;
                break;
        }

        posy = std::max(tposy, 0);
        posx = std::max(tposx, 0);
    }
}

void CMDFrame::setPosition(TextPosition pos)
{
    // set frame itself
    CMDBox::setPosition(pos);

    // recursively set the position of all children
    for (auto c_set = children; c_set != NULL; c_set = c_set->next) {
        for (auto box : c_set->members) box->setPosition(box->boxPosition);
    }
}

void CMDGrid::setPosition(TextPosition pos)
{
    int oldx = posx;
    int oldy = posy;
    CMDBox::setPosition(pos);
    int offsetx = posx - oldx;
    int offsety = posy - oldy;

    for (int y = 0; y < rows.count; ++y) {
        for (int x = 0; x < columns.count; ++x)
        {
            auto box = data[y][x];
            box->shift(offsetx, offsety);
        }
    }
}

char CMDBox::getCharIn(uint32_t x, uint32_t y)
{
	if (!isVisible) return 0;
	
    // out of bounds returns 0
    int minx = posx;
    int maxx = minx + width - 1;
    int miny = posy;
    int maxy = miny + height - 1;

    if (x < minx || x > maxx || y < miny || y > maxy) return 0;

    // check if border falls in coords
    if (bordered)
    {
        // corners
        if (x == minx && y == miny) return borders.topleft;
        if (x == minx && y == maxy) return borders.botmleft;
        if (x == maxx && y == miny) return borders.topright;
        if (x == maxx && y == maxy) return borders.botmright;

        // body
        if (x == minx) return borders.leftbody;
        if (x == maxx) return borders.rightbody;
        if (y == miny) return borders.topbody;
        if (y == maxy) return borders.botmbody;
    }

    // check if text falls in coords
    uint32_t textx = 0, texty = 0;
    // length of inner text
    uint32_t len = inner.length();

    switch (textPosition)
    {
        case TRUE_CENTER:
            texty = miny + height/2;
            textx = minx + width/2 - len/2;
            break;

        case CENTER_LEFT:
            texty = miny + height/2;
            textx = minx + (bordered ? 1 : 0);
            break;

        case CENTER_RIGHT:
            texty = miny + height/2;
            textx = maxx - len - (bordered ? 1 : 0);
            break;

        case TOP_CENTER:
            texty = miny + (bordered ? 1 : 0);
            textx = minx + width/2 - len/2;
            break;

        case TOP_LEFT:
            texty = miny + (bordered ? 1 : 0);
            textx = minx + (bordered ? 1 : 0);
            break;

        case TOP_RIGHT:
            texty = miny + (bordered ? 1 : 0);
            textx = maxx - len - (bordered ? 1 : 0);
            break;
            
        case BOTTOM_CENTER:
            texty = maxy - (bordered ? 1 : 0);
            textx = minx + width/2 - len/2;
            break;
            
        case BOTTOM_LEFT:
            texty = maxy - (bordered ? 1 : 0);
            textx = minx + (bordered ? 1 : 0);
            break;
            
        case BOTTOM_RIGHT:
            texty = maxy - (bordered ? 1 : 0);
            textx = maxx - len - (bordered ? 1 : 0);
            break;
    }

    // check if coords falls in inner
    if (y == texty && (x >= textx && x < textx + len)) return inner.at(x - textx);

    if (isTransparent) return 0; else return ' ';
}

void CMDFrame::updateDisplay(std::string elName)
{
    // update the display of a specific element
    auto el = getElementByName(elName);

    if (el != NULL) 
    {
        char buffer[el->width + 1];
        buffer[el->width] = 0;
        
        for (auto y = el->posy; y < el->posy + el->height; ++y)
        {
            setCursor(el->posx, y);
            for (auto x = 0; x < el->width; ++x)
            {
                char ch = getCharIn(el->posx + x, y);
                if (ch == 0) ch = ' ';
                buffer[x] = ch;
            }
            printf("%s", buffer);
        }
    }
}

void CMDFrame::updateDisplay(CMDBox* el)
{
    // update the display of a specific element
    // auto el = getElementByName(elName);

    if (el != NULL && isParentTo(el)) 
    {
        char buffer[el->width + 1];
        buffer[el->width] = 0;
        
        for (auto y = el->posy; y < el->posy + el->height; ++y)
        {
            setCursor(el->posx, y);
            for (auto x = 0; x < el->width; ++x)
            {
                char ch = getCharIn(el->posx + x, y);
                if (ch == 0) ch = ' ';
                buffer[x] = ch;
            }
            printf("%s", buffer);
        }
    }
}

char CMDFrame::getCharIn(uint32_t x, uint32_t y)
{
    auto ch = CMDBox::getCharIn(x, y);

    for (auto c_set = children; c_set != NULL && c_set->zindex >= 0; c_set = c_set->next)
    {
        bool found = false;
        for (auto box : c_set->members) 
        {
            char nch = box->getCharIn(x, y);
            if (nch != 0) 
            {
                ch = nch;
                found = true;
                break;
            }
        }

        if (found) break;
    }

    return ch;
}

char CMDGrid::getCharIn(uint32_t x, uint32_t y)
{
    // out of bounds returns 0
    int minx = posx;
    int maxx = minx + width - 1;
    int miny = posy;
    int maxy = miny + height - 1;

    if (x < minx || x > maxx || y < miny || y > maxy) return 0;

    // check if table border
    if (bordered)
    {
        for (int ty = 0; ty < rows.count; ++ty) 
        {
            for (int tx = 0; tx < columns.count; ++tx)
            {
                // check if x = cell->posx - 1 for any cells (left border)
                if (x == (data[ty][tx]->posx - 1)) return tableborderch;
                // check if y = cell->posy - 1 for any cells (top border)
                if (y == (data[ty][tx]->posy - 1)) return tableborderch;
                // check if x = cell->posx + cell->width for any cells (right border)
                if (x == (data[ty][tx]->posx + data[ty][tx]->width)) return tableborderch;
                // check if y = cell->posy + cell->height for any cells (bottom border)
                if (y == (data[ty][tx]->posy + data[ty][tx]->height)) return tableborderch;
            }
        }
    }

    // otherwise, must be a cell
    // find cell to add in (calculate based on loop)
    for (int ty = 0; ty < rows.count; ++ty) 
    {
        for (int tx = 0; tx < columns.count; ++tx)
        {
            // if quadrants land within square, then calculate recursively

            auto xmin = data[ty][tx]->posx;
            auto xmax = xmin + data[ty][tx]->width - 1;
            auto ymin = data[ty][tx]->posy;
            auto ymax = ymin + data[ty][tx]->height - 1;

            if (xmin <= x && x <= xmax && ymin <= y && y <= ymax)
            {
                return data[ty][tx]->getCharIn(x, y);
            }
        }
    }

    return 0;                       // added to assuage C++'s semchecker
}

CMDBox* CMDBox::getElementByName(std::string nom)
{
    if (name == nom) return this;
    else return NULL;
}

CMDBox* CMDFrame::getElementByName(std::string nom)
{
    if (CMDBox::getElementByName(nom) != NULL) return this;

    for (auto c_set = children; c_set != NULL; c_set = c_set->next)
    {
        for (auto box : c_set->members) 
        {
            auto res = box->getElementByName(nom);
            if (res != NULL) return res;
        }
    }

    return NULL;
}

CMDBox* CMDGrid::getElementByName(std::string nom)
{
    if (CMDBox::getElementByName(nom) != NULL) return this;

    for (auto y = 0; y < rows.count; ++y)
    {
        for (auto x = 0; x < columns.count; ++x)
        {
            auto cell = at(x, y);
            auto res = cell->getElementByName(nom);
            if (res != NULL) return res;
        }
    }

    return NULL;
}

bool CMDBox::isParentTo(CMDBox* addr) {return this == addr;}

bool CMDFrame::isParentTo(CMDBox* addr)
{
    if (addr == NULL) return false;
    if (this == addr) return true;

    for (auto c_set = children; c_set != NULL; c_set = c_set->next)
    {
        for (auto box : c_set->members) 
        {
            if (box == addr) return true;
            if (box->isParentTo(addr)) return true;
        }
    }

    return false;
}

bool CMDGrid::isParentTo(CMDBox* addr)
{
    if (addr == NULL) return false;
    if (this == addr) return true;

    for (auto y = 0; y < rows.count; ++y)
    {
        for (auto x = 0; x < columns.count; ++x)
        {
            if (at(x, y)->isParentTo(addr)) return true;
        }
    }

    return false;
}

void CMDFrame::display() 
{
    /*for (int i = 0; i < posy; ++i)
        std::cout << std::endl;*/

    for (int y = 0; y < height; ++y)
    {
        // space out
        //std::cout << std::string(posx, ' ');
        setCursor(posx, posy + y);

        // handle each char
        std::string line;
        for (int x = 0; x < width; ++x)
        {
            // attempt getCharIn on yourself
            char ch = getCharIn(x, y);

            if (ch == 0) ch = ' ';
            line.push_back(ch);
        }
        
        printf("%s\n", line.c_str());
    }

}

void CMDFrame::removeChild(CMDBox* child)
{
    if (child != NULL) 
    {
        bool isFound = false;
        for (auto c_set = children; c_set != NULL; c_set = c_set->next)
        {
            for (int i = 0; i < c_set->members.size(); ++i) 
            {
                auto box = c_set->members[i];
                if (box == child)
                {   
                    isFound = true;
                    c_set->members.erase(c_set->members.begin() + i);
                }
            }
        }

        if (isFound) {
            child->parent = NULL;
        }
    }
}

void CMDFrame::addChild(CMDBox *child, int zindex) 
{
    child->parent = this;

    if (children != NULL)
    {
        if (zindex < children->zindex) {
            
            auto level = children;
            while (level->next != NULL && zindex < level->zindex)
                level = level->next;
            
            if (level->zindex != zindex) {
                auto nlevel = new Indexing;
                nlevel->next = level;
                nlevel->prev = level->prev;
                level->prev = nlevel;
                nlevel->zindex = zindex;
                nlevel->members.push_back(child);
            }
            else level->members.push_back(child);
        }

        else if (zindex > children->zindex) {
            auto nlevel = new Indexing;
            nlevel->next = children;
            nlevel->prev = NULL;
            nlevel->zindex = zindex;
            nlevel->members.push_back(child);
            children->prev = nlevel;
            children = nlevel;
            return;
        }

        else children->members.push_back(child);
    }
    else 
    {
        children = new Indexing;
        children->next = NULL;
        children->prev = NULL;
        children->zindex = zindex;
        children->members.push_back(child);
    }
}

void CMDGrid::addRow()
{
    rows.count++;
    rows.rowheight.push_back(0);
    
    data.push_back(std::vector<CMDFrame*>());
    for (int x = 0; x < columns.count; ++x) 
    {
        auto nFrame = new CMDFrame("", columns.colwidth[x], rows.rowheight[rows.count-1]);
        nFrame->parent = this;
        nFrame->posx = (bordered) ? posx + 1 : posx;
        nFrame->posy = (rows.count == 1 ? (bordered ? posy + 1 : posy) : height + posy);
        data[rows.count-1].push_back(nFrame);
    }
}

void CMDGrid::addColumn()
{
    columns.count++;
    columns.colwidth.push_back(0);

    for (int y = 0; y < rows.count; ++y)
    {
        auto nFrame = new CMDFrame("", columns.colwidth[columns.count-1], rows.rowheight[y]);
        nFrame->parent = this;
        nFrame->posx = (columns.count == 1 ? (bordered ? posx + 1 : posx) : width + posx);
        nFrame->posy = (bordered) ? posy + 1 : posy;
        data[y].push_back(nFrame);
    }
}

void CMDGrid::deleteRow(uint32_t row)
{
    if (row < rows.count)
    {
        // adjust height
        auto rh = rows.rowheight[row];
        height -= rh;

        // account for border thickness
        if (bordered)
            height -= 1 + (rows.count < 2 ? 1 : 0);

        // delete row
        auto datum = data[row];
        data.erase(data.begin() + row);
        // delete all column entries in row
        for (int i = 0; i < columns.count; ++i)
            delete datum[i];

        // adjust row count
        rows.count--;
        // delete records of rowheight in rows
        rows.rowheight.erase(rows.rowheight.begin() + row);

        // adjust posy for remaining entries
        for (int y = row; y < rows.count; ++y)
            for (int x = 0; x < columns.count; ++x)
                data[y][x]->posy -= rh + (bordered ? 1 : 0);
    }
}

void CMDGrid::deleteColumn(uint32_t col)
{
    if (col < columns.count)
    {
        // adjust width
        auto cw = columns.colwidth[col];
        width -= cw;

        // account for border thickness
        if (bordered)
            width -= 1 + (columns.count < 2 ? 1 : 0);

        // delete column entries at col in all rows
        for (auto row : data)
        {
            auto datum = row[col];
            row.erase(row.begin() + col);
            delete datum;
        }

        // adjust column count
        columns.count--;
        // delete records of colwidth in rows
        columns.colwidth.erase(columns.colwidth.begin() + col);

        // adjust posx for remaining entries
        for (int y = 0; y < rows.count; ++y)
            for (int x = col; x < columns.count; ++x)
                data[y][x]->posx -= cw + (bordered ? 1 : 0);
    }
}

void CMDGrid::setHeight(uint32_t row, uint32_t hig)
{
    if (row < rows.count)
    {
        // adjust height of grid
        height += hig;
        height -= rows.rowheight[row];

        // account for border thickness
        if (bordered)
        {
            // if setting to zero from non-zero, remove borders
            if (hig == 0 && rows.rowheight[row] > 0)
                height -= 1 + (rows.count < 2 ? 1 : 0);

            // if setting to non-zero from zero, add borders
            if (hig > 0 && rows.rowheight[row] == 0)
                height += 1 + (rows.count < 2 ? 1 : 0);
        }

        // adjust height of all cells in row
        for (int i = 0; i < columns.count; ++i)
            data[row][i]->height = hig;

        // adjust posy for all lower cells
        for (int y = row + 1; y < rows.count; ++y) 
        {
            for (int x = 0; x < columns.count; ++x)
            {
                data[y][x]->posy += hig;
                data[y][x]->posy -= rows.rowheight[row];
            }
        }

        // update record
        rows.rowheight[row] = hig;
		
		// adjust position of grid again, as this might change due to the change in height
		setPosition(boxPosition);
    }
}

void CMDGrid::setWidth(uint32_t col, uint32_t wid)
{
    if (col < columns.count)
    {
        // adjust width of grid
        width += wid;
        width -= columns.colwidth[col];

        // account for border thickness
        if (bordered)
        {
            // if setting to zero from non-zero, remove borders
            if (wid == 0 && columns.colwidth[col] > 0)
                width -= 1 + (columns.count < 2 ? 1 : 0);

            // if setting to non-zero from zero, add borders
            if (wid > 0 && columns.colwidth[col] == 0)
                width += 1 + (columns.count < 2 ? 1 : 0);
        }

        // adjust width of all cells in column
        for (int i = 0; i < rows.count; ++i)
            data[i][col]->width = wid;

        // adjust posx for all rightward cells
        for (int y = 0; y < rows.count; ++y) 
        {
            for (int x = col + 1; x < columns.count; ++x)
            {
                data[y][x]->posx += wid;
                data[y][x]->posx -= columns.colwidth[col];
            }
        }

        // update record
        columns.colwidth[col] = wid;
		
		// adjust position of grid again, as this might change due to the change in width
		setPosition(boxPosition);
    }
}

void CMDGrid::addChild(CMDBox *child, int zindex, uint32_t row, uint32_t col)
{
    if (row < rows.count && col < columns.count)
    {
        // add child to correct cell
        auto cell = data[row][col];
        child->parent = cell;
        cell->addChild(child, zindex);

        // adjust cell size to accommodate child (add a parameter that adjusts this)
        if (sizeByContents)
        {
            // adjust row height based on maximum height in that row
            // adjust column width based on maximum width in that column
            for (int y = 0; y < rows.count; ++y)
            {
                for (int x = 0; x < columns.count; ++x)
                {
                    if (data[y][x]->height > rows.rowheight[y])
                        rows.rowheight[y] = data[y][x]->height;
                    if (data[y][x]->width > columns.colwidth[x])
                        columns.colwidth[x] = data[y][x]->width;
                }
            }
        }
    }
}

void CMDGrid::setBordered(bool isBordered)
{
    // do nothing if status is same
    if (bordered == isBordered) return;
    
    // otherwise, assume they are different
    // adjust posx and posy, as well as width and height, accordingly
    if (isBordered)
    {
        for (int y = 0; y < rows.count; ++y)
        {
            for (int x = 0; x < columns.count; ++x)
            {
                data[y][x]->posx += x + 1;
                data[y][x]->posy += y + 1;
            }
        }

        height = rows.count > 0 ? height + rows.count + 1 : 0;
        width = columns.count > 0 ? width + columns.count + 1 : 0;
    }
    else
    {
        for (int y = 0; y < rows.count; ++y)
        {
            for (int x = 0; x < columns.count; ++x)
            {
                data[y][x]->posx -= x + 1;
                data[y][x]->posy -= y + 1;
            }
        }

        height = rows.count > 0 ? height - (rows.count + 1) : 0;
        width = columns.count > 0 ? width - (columns.count + 1) : 0;
    }

    bordered = isBordered;
}

CMDFrame* CMDGrid::at(uint32_t x, uint32_t y)
{
    if (x < columns.count && y < rows.count)
        return data[y][x];
    else return NULL;
}