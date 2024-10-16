#pragma once
 
#include "cell.h"
#include "common.h"
 
#include <functional>
#include <vector>
 
class Sheet : public SheetInterface {
public:
    ~Sheet();
 
    void SetCell(Position pos, std::string text) override;
    
    CellInterface* GetCell(Position pos) override;
    const CellInterface* GetCell(Position pos) const override;
    Cell* GetConcreteCell(Position pos);
    const Cell* GetConcreteCell(Position pos) const;
 
    void ClearCell(Position pos) override;
 
    Size GetPrintableSize() const override;
 
    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;
 
private:
    std::vector<std::vector<std::unique_ptr<Cell>>> cells_;
    
    void MaybeIncreaseSizeToIncludePosition(Position pos);
    void PrintCells(std::ostream& output, const std::function<void(const CellInterface&)>& printCell) const;
};