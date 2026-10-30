(() => {
  'use strict';
  const ui=WorkspaceUI,root=document.getElementById('control-root');
  window.chrome.webview.addEventListener('message',event=>{
    if(event.data.kind==='state'){
      const state=event.data.state;ui.text(document.getElementById('tool-title'),state.title);ui.text(document.getElementById('tool-subtitle'),state.subtitle);
      document.querySelector('.desktop-app').classList.toggle('dialog-window',!!state.dialog);
      document.querySelector('.desktop-app').dataset.tool=state.tool;
      document.querySelector('.window-context').textContent=state.dialog?'LAB PLUS':'RESEARCH WORKSPACE';ui.renderControls(root,state.tree);
      ui.rendered();
    }else if(event.data.kind==='error')ui.text(document.getElementById('feedback'),event.data.text);
  });
  ui.wireWindow();ui.ready();
})();
