#include "formula.h"
 
#include "FormulaAST.h"
 
#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
 
using namespace std::literals;

    FormulaError::FormulaError(Category category) {
        category_ = category;
    }

    FormulaError::Category FormulaError::GetCategory() const {
        return category_;
    }

    bool FormulaError::operator==(FormulaError rhs) const {
        return category_ == rhs.category_;
    }

    std::string_view FormulaError::ToString() const {
        switch (category_) {
        case Category::Ref:
            return "#REF!";
        case Category::Value:
            return "#VALUE!";
        case Category::Arithmetic:
            return "#ARITHM!";
        }
        return "";
    }
 
std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}
 
namespace {
class Formula : public FormulaInterface {
public:
    
    explicit Formula(std::string expression) 
        try : ast_(ParseFormulaAST(expression)) {
        Position prev = Position::NONE;
            for (auto& cell : ast_.GetCells()) {
                if (cell.IsValid() && !(cell == prev)) {
                referenced_cells_.push_back(std::move(cell));
                prev = cell;
                }
            }
        }
        catch (...) {
            throw FormulaException("formula is syntactically incorrect");
        }
    
    Value Evaluate(const SheetInterface& sheet) const override { 
        try {
            std::function<double(Position)> get_cell_value = [&sheet](const Position pos) -> double {
                if (!pos.IsValid()) {
                    throw FormulaError(FormulaError::Category::Ref);
                }

                const auto* cell = sheet.GetCell(pos);
                if (!cell) {
                    return 0.0;
                }

                const auto& value = cell->GetValue();

                if (std::holds_alternative<double>(value)) {
                    return std::get<double>(value);
                }
                else if (std::holds_alternative<std::string>(value)) {
                    const auto& str_value = std::get<std::string>(value);
                    if (str_value.empty()) {
                        return 0.0;
                    }

                    std::istringstream input(str_value);
                    double num;
                    if (input >> num && input.eof()) {
                        return num;
                    }
                    else {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                }
                else {
                    throw std::get<FormulaError>(value);
                }
                };

            return ast_.Execute(get_cell_value);
        }
        catch (const FormulaError& err) {
            return err;
        }
    }
    
    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);       
        return out.str();
    }
    
    std::vector<Position> GetReferencedCells() const override {
        return referenced_cells_;
    }
 
private:
    FormulaAST ast_;
    std::vector<Position> referenced_cells_;
};
    
}//end namespace
 
std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}