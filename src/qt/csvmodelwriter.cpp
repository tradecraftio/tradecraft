// Copyright (c) 2011-2014 The Bitcoin Core developers
// Copyright (c) 2011-2019 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or
// modify it under the conjunctive terms of BOTH version 3 of the GNU
// Affero General Public License as published by the Free Software
// Foundation AND the MIT/X11 software license.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License and the MIT/X11 software license for
// more details.
//
// You should have received a copy of both licenses along with this
// program.  If not, see <https://www.gnu.org/licenses/> and
// <http://www.opensource.org/licenses/mit-license.php>

#include "csvmodelwriter.h"

#include <QAbstractItemModel>
#include <QFile>
#include <QTextStream>

CSVModelWriter::CSVModelWriter(const QString &filename, QObject *parent) :
    QObject(parent),
    filename(filename), model(0)
{
}

void CSVModelWriter::setModel(const QAbstractItemModel *model)
{
    this->model = model;
}

void CSVModelWriter::addColumn(const QString &title, int column, int role)
{
    Column col;
    col.title = title;
    col.column = column;
    col.role = role;

    columns.append(col);
}

static void writeValue(QTextStream &f, const QString &value)
{
    QString escaped = value;
    escaped.replace('"', "\"\"");
    f << "\"" << escaped << "\"";
}

static void writeSep(QTextStream &f)
{
    f << ",";
}

static void writeNewline(QTextStream &f)
{
    f << "\n";
}

bool CSVModelWriter::write()
{
    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&file);

    int numRows = 0;
    if(model)
    {
        numRows = model->rowCount();
    }

    // Header row
    for(int i=0; i<columns.size(); ++i)
    {
        if(i!=0)
        {
            writeSep(out);
        }
        writeValue(out, columns[i].title);
    }
    writeNewline(out);

    // Data rows
    for(int j=0; j<numRows; ++j)
    {
        for(int i=0; i<columns.size(); ++i)
        {
            if(i!=0)
            {
                writeSep(out);
            }
            QVariant data = model->index(j, columns[i].column).data(columns[i].role);
            writeValue(out, data.toString());
        }
        writeNewline(out);
    }

    file.close();

    return file.error() == QFile::NoError;
}
