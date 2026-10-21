(() => {
  'use strict';
  const reviews = window.READING_REVIEWS;
  const notebook = document.getElementById('notebook');
  const left = document.getElementById('leftPage');
  const right = document.getElementById('rightPage');
  const prev = document.getElementById('prevPage');
  const next = document.getElementById('nextPage');
  const full = document.getElementById('fullReviews');
  const fullToggle = document.getElementById('fullTextToggle');
  const indexButtons = [...document.querySelectorAll('[data-book]')];
  const mobile = window.matchMedia('(max-width:700px)');
  const reduceMotion = window.matchMedia('(prefers-reduced-motion:reduce)');
  let pages = [], current = 0, layoutTimer, lastWidth = 0;
  const step = () => mobile.matches ? 1 : 2;

  document.addEventListener('click', event => {
    const button = event.target.closest('[data-print]');
    if (!button) return;
    document.body.dataset.printMode = button.dataset.print || 'profile';
    window.print();
  });
  window.addEventListener('afterprint', () => { delete document.body.dataset.printMode; });
  if (!reviews || !notebook) return;

  const paragraph = (text, className) => {
    const node = document.createElement('p');
    node.textContent = text;
    if (className) node.className = className;
    return node;
  };
  function bookHeader(book, continuation) {
    const header = document.createElement('header');
    if (continuation) {
      header.append(paragraph(`《${book.title}》 · 续`, 'continued-heading'));
    } else {
      const kicker = document.createElement('span');
      kicker.className = 'book-kicker';
      kicker.textContent = 'READING NOTES / 读书记录';
      const title = document.createElement('h3');
      title.textContent = `《${book.title}》`;
      header.append(kicker, title, paragraph(`${book.author}\n${book.dates}`, 'book-meta'));
    }
    return header;
  }
  function paginate() {
    const oldPage = pages[current];
    const oldPart = oldPage?.parts[0];
    const anchor = oldPage && { book: oldPage.book.id, block: oldPart?.block ?? 0, offset: oldPart?.start ?? 0 };
    document.body.classList.add('journal-ready');
    const width = left.clientWidth;
    const height = left.clientHeight;
    if (!width || !height) return;
    const measure = document.createElement('div');
    measure.className = 'page-content measure-page';
    Object.assign(measure.style, { width: `${width}px`, height: `${height}px` });
    notebook.append(measure);
    const result = [];
    let page;
    function begin(book, continuation) {
      measure.replaceChildren(bookHeader(book, continuation));
      page = { book, continuation, parts: [] };
    }
    function finish() {
      if (page.parts.length) result.push(page);
    }
    function fits(node) {
      measure.append(node);
      const fitsPage = measure.scrollHeight <= measure.clientHeight + 1;
      node.remove();
      return fitsPage;
    }
    for (const book of reviews) {
      begin(book, false);
      const blocks = book.paragraphs.map(text => ({ text, quote: false }));
      if (book.quote) blocks.push({ text: book.quote, quote: true });
      blocks.forEach((block, blockIndex) => {
        let offset = 0;
        while (offset < block.text.length) {
          const node = document.createElement(block.quote ? 'blockquote' : 'p');
          if (offset) node.className = 'continued';
          const remaining = block.text.slice(offset);
          node.textContent = remaining;
          let count = remaining.length;
          if (!fits(node)) {
            let low = 0, high = remaining.length;
            while (low < high) {
              const middle = Math.ceil((low + high) / 2);
              node.textContent = remaining.slice(0, middle);
              if (fits(node)) low = middle; else high = middle - 1;
            }
            count = low;
            if (count < 18 && page.parts.length) {
              finish(); begin(book, true); continue;
            }
            // Avoid starting a new page with closing punctuation.
            if (count < remaining.length && /[，。！？、；：”’）》]/.test(remaining[count]) && count > 2) count -= 1;
          }
          if (!count) throw new Error('The reading page is too small to fit text.');
          node.textContent = remaining.slice(0, count);
          measure.append(node);
          page.parts.push({ text: node.textContent, quote: block.quote, block: blockIndex, start: offset, continued: offset > 0 });
          offset += count;
          if (offset < block.text.length) { finish(); begin(book, true); }
        }
      });
      finish();
    }
    measure.remove();
    pages = result;
    current = 0;
    if (anchor) {
      const found = pages.findIndex(p => p.book.id === anchor.book && p.parts.some(part => part.block === anchor.block && part.start <= anchor.offset && part.start + part.text.length > anchor.offset));
      if (found >= 0) current = found;
    }
    lastWidth = notebook.clientWidth;
    render();
    full.hidden = fullToggle.getAttribute('aria-expanded') !== 'true';
  }
  function renderPage(target, page, number) {
    target.replaceChildren();
    if (!page) {
      target.append(paragraph('先读到这里。', 'continued-heading'));
      target.dataset.book = '';
      return;
    }
    target.dataset.book = page.book.id;
    target.append(bookHeader(page.book, page.continuation));
    for (const part of page.parts) {
      const node = document.createElement(part.quote ? 'blockquote' : 'p');
      if (part.continued) node.className = 'continued';
      node.textContent = part.text;
      node.dataset.block = String(part.block);
      node.dataset.start = String(part.start);
      target.append(node);
    }
    target.setAttribute('aria-label', `《${page.book.title}》，第 ${number} 页`);
  }
  function render() {
    renderPage(left, pages[current], current + 1);
    renderPage(right, mobile.matches ? null : pages[current + 1], current + 2);
    document.getElementById('leftNumber').textContent = String(current + 1).padStart(2, '0');
    document.getElementById('rightNumber').textContent = pages[current + 1] ? String(current + 2).padStart(2, '0') : '';
    prev.disabled = current === 0;
    next.disabled = current + step() >= pages.length;
    const range = !mobile.matches && pages[current + 1] ? `${current + 1}–${current + 2}` : `${current + 1}`;
    document.getElementById('pageStatus').textContent = `《${pages[current].book.title}》 · ${range} / ${pages.length} 页`;
    notebook.dataset.totalPages = String(pages.length);
    notebook.dataset.currentPage = String(current);
    indexButtons.forEach(button => {
      const selected = button.dataset.book === pages[current].book.id;
      if (selected) button.setAttribute('aria-current', 'true'); else button.removeAttribute('aria-current');
    });
  }
  function turn(direction) {
    if ((direction < 0 && prev.disabled) || (direction > 0 && next.disabled)) return;
    notebook.querySelectorAll('.turning-page').forEach(node => node.remove());
    let overlay;
    if (!reduceMotion.matches) {
      const source = mobile.matches || direction < 0 ? left.parentElement : right.parentElement;
      overlay = source.cloneNode(true);
      overlay.removeAttribute('id');
      overlay.querySelectorAll('[id]').forEach(node => node.removeAttribute('id'));
      overlay.className = `paper-page turning-page ${direction > 0 ? 'forward' : 'backward'}`;
      overlay.setAttribute('aria-hidden', 'true');
    }
    current = Math.min(pages.length - 1, Math.max(0, current + direction * step()));
    render();
    if (overlay) {
      notebook.append(overlay);
      overlay.addEventListener('animationend', () => overlay.remove(), { once: true });
      window.setTimeout(() => overlay.remove(), 600);
    }
  }
  prev.addEventListener('click', () => turn(-1));
  next.addEventListener('click', () => turn(1));
  indexButtons.forEach(button => button.addEventListener('click', () => {
    const page = pages.findIndex(item => item.book.id === button.dataset.book);
    if (page >= 0) { current = page; notebook.querySelectorAll('.turning-page').forEach(node => node.remove()); render(); }
  }));
  document.getElementById('reading').addEventListener('keydown', event => {
    if (event.target.closest('.full-reviews') || !['ArrowLeft','ArrowRight'].includes(event.key)) return;
    if (!pages.length) return;
    event.preventDefault();
    turn(event.key === 'ArrowRight' ? 1 : -1);
  });
  let touchStart;
  notebook.addEventListener('touchstart', event => {
    if (event.touches.length === 1) touchStart = { x: event.touches[0].clientX, y: event.touches[0].clientY };
  }, { passive: true });
  notebook.addEventListener('touchend', event => {
    if (!touchStart) return;
    const touch = event.changedTouches[0];
    const dx = touch.clientX - touchStart.x, dy = touch.clientY - touchStart.y;
    if (Math.abs(dx) > 55 && Math.abs(dx) > Math.abs(dy) * 1.6 && pages.length) turn(dx < 0 ? 1 : -1);
    touchStart = null;
  }, { passive: true });
  notebook.addEventListener('touchcancel', () => { touchStart = null; }, { passive: true });
  fullToggle.addEventListener('click', () => {
    const expanded = fullToggle.getAttribute('aria-expanded') !== 'true';
    fullToggle.setAttribute('aria-expanded', String(expanded));
    fullToggle.textContent = expanded ? '收起全文 ↑' : '展开全文，慢慢读 ↓';
    full.hidden = !expanded;
  });
  const print = document.createElement('button');
  print.type = 'button'; print.className = 'text-link'; print.dataset.print = 'reviews'; print.textContent = '打印书评 ↗';
  document.querySelector('.reading-tools').append(print);
  function safePaginate() {
    try { paginate(); }
    catch (error) {
      document.body.classList.remove('journal-ready');
      full.hidden = false;
      fullToggle.hidden = true;
      console.error('Reading notebook layout failed; full text remains available.', error);
    }
  }
  safePaginate();
  document.fonts?.ready.then(safePaginate);
  const observer = new ResizeObserver(() => {
    if (Math.abs(notebook.clientWidth - lastWidth) < 2) return;
    clearTimeout(layoutTimer);
    layoutTimer = setTimeout(safePaginate, 120);
  });
  observer.observe(notebook);
  mobile.addEventListener('change', safePaginate);
})();
