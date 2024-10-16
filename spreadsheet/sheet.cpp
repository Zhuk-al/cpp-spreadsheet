#include "sheet.h"
 
#include "cell.h"
#include "common.h"
 
#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
 
using namespace std::literals;
 
Sheet::~Sheet() {}
 
void Sheet::SetCell(Position pos, std::string text) { 
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid cell position");
    }

    MaybeIncreaseSizeToIncludePosition(pos);

    if (!cells_[pos.row][pos.col]) {
        cells_[pos.row][pos.col] = std::make_unique<Cell>(*this);
    }
    
    cells_[pos.row][pos.col]->Set(std::move(text)); 
}
 
CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid cell position");
    }

    return GetConcreteCell(pos);
}
 
const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid cell position");
    }

    return GetConcreteCell(pos);
}
 
Cell* Sheet::GetConcreteCell(Position pos) {
    if (pos.IsValid() && pos.row < int(cells_.size()) && pos.col < int(cells_[pos.row].size())) {
        return cells_[pos.row][pos.col].get();
    }
    return nullptr;
    
}
 
const Cell* Sheet::GetConcreteCell(Position pos) const {
    if (pos.IsValid() && pos.row < int(cells_.size()) && pos.col < int(cells_[pos.row].size())) {
        return cells_[pos.row][pos.col].get();
    }
    return nullptr;
}
 
void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid cell position");
    }

    MaybeIncreaseSizeToIncludePosition(pos);

    if (cells_[pos.row][pos.col]) {
        cells_[pos.row][pos.col]->Clear();
        cells_[pos.row][pos.col].reset();
    }
}
 
Size Sheet::GetPrintableSize() const {    
    Size size;   
    for (int row = 0; row < int(std::size(cells_)); ++row) {       
        for (int col = (int(std::size(cells_[row])) - 1); col >= 0; --col) {
            if (cells_[row][col]) {             
                if (cells_[row][col]->GetText().empty()) {
                    continue;                
                } else {
                    size.rows = std::max(size.rows, row + 1);
                    size.cols = std::max(size.cols, col + 1);
                    break;
                }
            }
        }
    }  
    return size;
}
 
void Sheet::PrintValues(std::ostream& output) const {
    PrintCells(output, [&output](const CellInterface& cell) {
        std::visit([&output](const auto& value) { output << value; }, cell.GetValue());
    });
}
            
void Sheet::PrintTexts(std::ostream& output) const {
    PrintCells(output, [&output](const CellInterface& cell) { output << cell.GetText(); });
}

void Sheet::MaybeIncreaseSizeToIncludePosition(Position pos) {
    if (pos.row >= int(cells_.size())) {
        cells_.resize(pos.row + 1);
    }
    if (pos.col >= int(cells_[pos.row].size())) {
        cells_[pos.row].resize(pos.col + 1);
    }
}

void Sheet::PrintCells(std::ostream& output, const std::function<void(const CellInterface&)>& printCell) const {
    Size size = GetPrintableSize();

    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }
            if (col < int(cells_[row].size()) && cells_[row][col]) {
                printCell(*cells_[row][col]);
            }
        }
        output << '\n';
    }
}
 
std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}