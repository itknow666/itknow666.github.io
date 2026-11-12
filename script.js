// Optional. In-page links still work if this file does not run.
// Keeps section headings below the sticky header after the nav wraps.
(function () {
  var header = document.querySelector(".site-header");
  if (!header) return;

  function syncHeaderOffset() {
    document.documentElement.style.setProperty("--header-h", header.offsetHeight + "px");
  }

  syncHeaderOffset();
  window.addEventListener("resize", syncHeaderOffset);
  if (document.fonts && document.fonts.ready) document.fonts.ready.then(syncHeaderOffset);
})();
