//! Reusable native priority order: lowest cost first; relaxed equal costs are FIFO.
//! Seeds append directly and therefore pop newest first before relaxation.

use std::collections::TryReserveError;

#[derive(Debug)]
struct Item<T> {
    value: T,
    cost: i32,
}
#[derive(Debug)]
pub(crate) struct Worklist<T> {
    items: Vec<Item<T>>,
}
impl<T> Default for Worklist<T> {
    fn default() -> Self {
        Self { items: Vec::new() }
    }
}
impl<T> Worklist<T> {
    // Native seeds append; several zero-cost seeds therefore pop newest first.
    // Relaxed entries still use oldest-equal ordered insertion below.
    pub(crate) fn seed(&mut self, value: T) -> Result<(), TryReserveError> {
        self.items.try_reserve(1)?;
        self.items.push(Item { value, cost: 0 });
        Ok(())
    }
    pub(crate) fn clear(&mut self) {
        self.items.clear();
    }
    pub(crate) fn insert(&mut self, value: T, cost: i32) -> Result<(), TryReserveError> {
        // Insert before existing equals in descending order, then pop the back.
        let at = self.items.partition_point(|item| cost < item.cost);
        self.items.try_reserve(1)?;
        self.items.insert(at, Item { value, cost });
        Ok(())
    }
    pub(crate) fn pop(&mut self) -> Option<T> {
        self.items.pop().map(|item| item.value)
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn equal_costs_remain_fifo_including_insertions_during_search() {
        let mut queue = Worklist::default();
        queue.insert('a', 2).unwrap();
        queue.insert('b', 1).unwrap();
        queue.insert('c', 2).unwrap();
        queue.insert('d', 1).unwrap();
        assert_eq!(queue.pop(), Some('b'));
        queue.insert('e', 1).unwrap();
        assert_eq!(queue.pop(), Some('d'));
        assert_eq!(queue.pop(), Some('e'));
        assert_eq!(queue.pop(), Some('a'));
        assert_eq!(queue.pop(), Some('c'));
        let capacity = queue.items.capacity();
        queue.clear();
        queue.insert('f', -1).unwrap();
        assert_eq!(queue.items.capacity(), capacity);
        assert_eq!(queue.pop(), Some('f'));
        assert_eq!(queue.pop(), None);
    }
}
