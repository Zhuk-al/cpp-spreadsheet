#include "cell.h"
#include "sheet.h"
 
#include <cassert>
#include <iostream>
#include <string>
#include <optional>
 
Cell::Cell(Sheet& sheet) 
    : impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet) 
{}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    std::unique_ptr<Impl> new_impl;

    if (text.empty()) {
        new_impl = std::make_unique<EmptyImpl>();
    } 
    else if (text[0] == FORMULA_SIGN) {
        new_impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
    } 
    else {
        new_impl = std::make_unique<TextImpl>(std::move(text));
    }

    // Проверка на циклическую зависимость
    CheckCircularDependency(new_impl);

    // Обновление зависимостей
    UpdateDependencies(*new_impl);

    // Обновление кэша
    UpdateCache(std::move(new_impl));
}

// Метод для обновления зависимостей
void Cell::UpdateDependencies(const Impl& new_impl) {
    // Очистка старых зависимостей
    for (Cell* ref : referenced_cells_) {
        ref->dependent_cells_.erase(this);
    }
    referenced_cells_.clear();

    // Установка новых зависимостей
    for (const auto& pos : new_impl.GetReferencedCells()) {
        Cell* ref_cell = sheet_.GetConcreteCell(pos);
        if (!ref_cell) {
            sheet_.SetCell(pos, "");
            ref_cell = sheet_.GetConcreteCell(pos);
        }
        referenced_cells_.insert(ref_cell);
        ref_cell->dependent_cells_.insert(this);
    }
}

// Метод для обновления кэша
void Cell::UpdateCache(std::unique_ptr<Impl> new_impl) {
    impl_ = std::move(new_impl);
    InvalidateAllCache(true);
}

void Cell::Clear() {
    Set("");
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !dependent_cells_.empty();
}

void Cell::InvalidateAllCache(bool flag = false) {
    if (impl_->HasCache() || flag) {
        impl_->InvalidateCache();
        
        for (Cell* dependent : dependent_cells_) {
            dependent->InvalidateAllCache();
        }
    }
}

bool Cell::CheckCircularDependency(const std::unique_ptr<Impl>& impl) {
    std::set<const Cell*> visited;
    std::stack<const Cell*> to_visit;
    to_visit.push(this);

    const auto& ref_cells = impl->GetReferencedCells();
    for (const auto& pos : ref_cells) {
        Cell* ref_cell = sheet_.GetConcreteCell(pos);
        if (ref_cell == this) {
            throw CircularDependencyException("Circular dependency detected"); // Циклическая зависимость
        }
        to_visit.push(ref_cell);
    }

    while (!to_visit.empty()) {
        const Cell* curr = to_visit.top();
        to_visit.pop();

        if (visited.count(curr)) {
            continue;
        }
        visited.insert(curr);

        for (const Cell* dep : curr->dependent_cells_) {
            if (dep == this) {
                throw CircularDependencyException("Circular dependency detected"); // Циклическая зависимость
            }
            to_visit.push(dep);
        }
    }
    return false;
}

// Реализация классов Impl, EmptyImpl, TextImpl, FormulaImpl

std::vector<Position> Cell::Impl::GetReferencedCells() const {
    return {}; 
}

bool Cell::Impl::HasCache() {
    return true;
}

void Cell::Impl::InvalidateCache() {}

Cell::Value Cell::EmptyImpl::GetValue() const {
    return "";
}

std::string Cell::EmptyImpl::GetText() const {
    return "";
}

Cell::TextImpl::TextImpl(std::string text) : text_(std::move(text)) {}
Cell::Value Cell::TextImpl::GetValue() const {
    if (text_.empty()) {
        return "";
    }
    else if (text_[0] == ESCAPE_SIGN) {
        return text_.substr(1);
    }
    else {
        return text_;
    }
}
std::string Cell::TextImpl::GetText() const {
    return text_;
}

Cell::FormulaImpl::FormulaImpl(std::string text, SheetInterface& sheet)
    : formula_ptr_(ParseFormula(text.substr(1)))
    , sheet_(sheet) 
{}

Cell::Value Cell::FormulaImpl::GetValue() const {                
    if (!cache_) {
        cache_ = formula_ptr_->Evaluate(sheet_);
    }     
    return std::visit([](auto& val) {
        return Value(val);
    }, *cache_);        
}

std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + formula_ptr_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_ptr_->GetReferencedCells();
}

bool Cell::FormulaImpl::HasCache() {
    return cache_.has_value();
}

void Cell::FormulaImpl::InvalidateCache() {
    cache_.reset();
}