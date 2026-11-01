(() => {
  'use strict';
  const send = message => window.chrome.webview.postMessage(message);
  const nodes = new Map();
  function element(tag, className) { const e = document.createElement(tag); if (className) e.className = className; return e; }
  function control(id, action, values = {}) { send({kind:'control', id, action, ...values}); }
  function text(e, value) { const next = String(value ?? ''); if(e.textContent !== next)e.textContent=next; }
  function value(e, next) { if(document.activeElement !== e && e.value !== String(next ?? ''))e.value=next ?? ''; }
  function appendChildren(parent, children) {
    const desired = children.map(node => render(node));
    for(let i=0;i<desired.length;i++)if(parent.children[i]!==desired[i])parent.insertBefore(desired[i],parent.children[i]||null);
    for(const child of Array.from(parent.children))if(!desired.includes(child))child.remove();
  }
  function create(n) {
    let e;
    switch(n.kind) {
      case 'button': e=element('button','lr-btn');e.type='button';e.addEventListener('click',()=>control(n.id,'click'));break;
      case 'label':e=element('div');break;
      case 'check':case 'radio': {
        e=element('label','control-check');const input=element('input');input.type=n.kind==='radio'?'radio':'checkbox';const span=element('span');e.append(input,span);
        input.addEventListener('change',()=>control(n.id,'check',{value:input.checked}));break;
      }
      case 'text':case 'combo':case 'number':case 'textarea': {
        e=element(n.kind==='textarea'?'textarea':'input','control-field');
        if(n.kind==='number')e.type='number';
        if(n.kind==='combo'){const list=element('datalist');list.id='list-'+n.id;document.body.append(list);e.setAttribute('list',list.id);}
        let debounce;
        e.addEventListener('input',()=>{clearTimeout(debounce);debounce=setTimeout(()=>{if(n.kind==='number'){if(e.value!==''&&Number.isFinite(Number(e.value)))control(n.id,'number',{value:Number(e.value)});}else control(n.id,'text',{value:e.value});},70);});
        e.addEventListener('change',()=>{clearTimeout(debounce);if(n.kind==='number'){if(e.value!=='')control(n.id,'number',{value:Number(e.value)});}else control(n.id,'text',{value:e.value});});break;
      }
      case 'select':e=element('select','control-select');e.addEventListener('change',()=>control(n.id,'select',{value:e.selectedIndex}));break;
      case 'slider':e=element('input','control-slider');e.type='range';e.addEventListener('input',()=>control(n.id,'number',{value:Number(e.value)}));break;
      case 'table':e=element('div','control-grid-table');break;
      case 'checked-list':case 'list':e=element('div','control-list');break;
      case 'canvas': {
        e=element('div','control-canvas');const image=element('img');image.draggable=false;image.alt='Interactive research plot';e.append(image);
        const mouse=(phase,event)=>{const r=image.getBoundingClientRect();const button=phase==='up'||phase==='click'?(event.button===2?2:event.button===0?1:0):(event.buttons&2)?2:(event.buttons&1)?1:0;control(n.id,'mouse',{phase,x:(event.clientX-r.left)/Math.max(1,r.width),y:(event.clientY-r.top)/Math.max(1,r.height),button,delta:phase==='wheel'?-Math.sign(event.deltaY)*120:0});};
        image.addEventListener('pointerdown',event=>{event.preventDefault();image.setPointerCapture(event.pointerId);mouse('down',event);});
        let moved=0;image.addEventListener('pointermove',event=>{if(performance.now()-moved<32)return;moved=performance.now();mouse('move',event);});
        image.addEventListener('pointerup',event=>mouse('up',event));image.addEventListener('pointercancel',event=>mouse('up',event));image.addEventListener('click',event=>mouse('click',event));image.addEventListener('wheel',event=>{event.preventDefault();mouse('wheel',event);},{passive:false});image.addEventListener('contextmenu',event=>event.preventDefault());break;
      }
      case 'tree':e=element('div','control-tree');break;
      case 'group':e=element('fieldset','control-group');e.append(element('legend'),element('div','control-column'));break;
      case 'tabs':e=element('div');e.append(element('div','control-tab-nav'),element('div','control-column'));break;
      default:e=element('div');break;
    }
    e.dataset.control=n.id;e.dataset.kind=n.kind;nodes.set(n.id,e);return e;
  }
  function render(n) {
    const e=nodes.get(n.id)||create(n);e.classList.add('control-node');
    e.setAttribute('aria-disabled',String(!n.enabled));
    if(n.column)e.style.gridColumn=n.column+' / span '+(n.span||1);
    if(n.row)e.style.gridRow=n.row;
    if('disabled' in e)e.disabled=!n.enabled;
    switch(n.kind) {
      case 'button':text(e,n.text);e.classList.toggle('lr-primary',!!n.primary);break;
      case 'label':
        if(n.collapsible){if(!e.firstChild||e.firstChild.tagName!=='DETAILS'){const details=element('details'),summary=element('summary'),body=element('div','control-caption');text(summary,'Notes and limits');details.append(summary,body);e.replaceChildren(details);}text(e.firstChild.lastChild,n.text);e.className='control-node control-notes';}
        else{text(e,n.text);e.className='control-node '+(n.heading?'control-title':n.small?'control-caption':'control-label');}break;
      case 'check':case 'radio':e.firstChild.checked=!!n.checked;e.firstChild.disabled=!n.enabled;text(e.lastChild,n.text);break;
      case 'text':case 'combo':case 'number':case 'textarea':
        value(e,n.value);e.readOnly=!!n.readonly;e.placeholder=n.placeholder||'';
        if(n.maxLength>0)e.maxLength=n.maxLength;
        if(n.kind==='number'){e.min=n.minimum;e.max=n.maximum;e.step=n.step;}
        if(n.kind!=='textarea')e.style.width=n.fill?'100%':Math.min(740,Math.max(120,n.width||180))+'px';
        if(n.kind==='textarea'){e.style.minHeight=Math.min(250,Math.max(110,n.height||110))+'px';e.wrap=n.wrap?'soft':'off';e.style.whiteSpace=n.wrap?'pre-wrap':'pre';}
        if(n.kind==='combo'){const list=document.getElementById('list-'+n.id);const signature=JSON.stringify(n.items);if(list.dataset.signature!==signature){list.replaceChildren(...n.items.map(item=>{const o=element('option');o.value=item;return o;}));list.dataset.signature=signature;}}break;
      case 'select': {
        const signature=JSON.stringify(n.items);if(e.dataset.signature!==signature){e.replaceChildren(...n.items.map(item=>{const o=element('option');text(o,item);return o;}));e.dataset.signature=signature;}
        if(document.activeElement!==e)e.selectedIndex=n.selected;break;
      }
      case 'slider':e.min=n.minimum;e.max=n.maximum;e.step=n.step||1;value(e,n.value);break;
      case 'table':renderTable(e,n);break;
      case 'list':case 'checked-list':renderList(e,n);e.querySelectorAll('input').forEach(input=>input.disabled=!n.enabled);break;
      case 'canvas':if(n.image&&e.firstChild.src!==n.image)e.firstChild.src=n.image;if(!e.children[1])e.append(element('div','canvas-actions'));appendChildren(e.children[1],n.actions||[]);break;
      case 'tree':renderTree(e,n.nodes,!!n.checks);e.querySelectorAll('input').forEach(input=>input.disabled=!n.enabled);break;
      case 'tabs': {
        const nav=e.firstChild;
        if(nav.children.length!==n.items.length)nav.replaceChildren(...n.items.map((label,i)=>{const b=element('button');b.type='button';text(b,label);b.addEventListener('click',()=>control(n.id,'select',{value:i}));return b;}));
        Array.from(nav.children).forEach((b,i)=>b.classList.toggle('selected',i===n.selected));appendChildren(e.lastChild,n.children||[]);break;
      }
      case 'group':text(e.firstChild,n.text);appendChildren(e.lastChild,n.children||[]);break;
      default:
        e.className='control-node '+(n.kind==='toolbar'?'control-toolbar':n.kind==='grid'||n.kind==='split'?'control-grid':'control-column');
        if(n.kind==='grid')e.style.gridTemplateColumns=(n.columns||['1fr']).join(' ');
        if(n.kind==='split')e.style.gridTemplateColumns=n.vertical?`minmax(180px,${n.ratio}fr) minmax(0,${1-n.ratio}fr)`:'1fr';
        appendChildren(e,n.children||[]);break;
    }
    return e;
  }
  function renderTable(e,n) {
    const signature=JSON.stringify([n.columns,n.rows.map(row=>[row.index,row.values]),n.total]);
    if(e.dataset.signature===signature){Array.from(e.querySelectorAll('tbody tr')).forEach((tr,i)=>tr.classList.toggle('selected',!!n.rows[i]?.selected));return;}
    const top=e.scrollTop,left=e.scrollLeft;const table=element('table'),head=element('thead'),hr=element('tr'),body=element('tbody');
    n.columns.forEach(c=>{const th=element('th');text(th,c.text);hr.append(th);});head.append(hr);table.append(head,body);
    n.rows.forEach(row=>{const tr=element('tr');tr.tabIndex=0;tr.classList.toggle('selected',row.selected);tr.addEventListener('keydown',event=>{if(event.key==='Enter'){event.preventDefault();control(n.id,'row-double',{row:row.index,column:n.columns[0].index});}else if(event.key==='ArrowDown'||event.key==='ArrowUp'){const next=event.key==='ArrowDown'?tr.nextElementSibling:tr.previousElementSibling;if(next){event.preventDefault();next.focus();control(n.id,'row',{row:n.rows[Array.from(body.children).indexOf(next)].index,column:n.columns[0].index});}}});row.values.forEach((cell,i)=>{const td=element('td');text(td,cell);td.title=String(cell);td.addEventListener('click',()=>control(n.id,'row',{row:row.index,column:n.columns[i].index}));td.addEventListener('dblclick',()=>control(n.id,'row-double',{row:row.index,column:n.columns[i].index}));tr.append(td);});body.append(tr);});
    if(!n.rows.length){const row=element('tr','empty-row'),cell=element('td');cell.colSpan=Math.max(1,n.columns.length);text(cell,n.empty);row.append(cell);body.append(row);}
    e.replaceChildren(table);
    if(n.total>n.rows.length){const more=element('button','lr-btn');more.type='button';text(more,`Show more (${n.rows.length} of ${n.total})`);more.addEventListener('click',()=>control(n.id,'more'));e.append(more);}
    e.dataset.signature=signature;e.scrollTop=top;e.scrollLeft=left;
  }
  function renderList(e,n) {
    const signature=JSON.stringify([n.items,n.checks,n.selected]);if(e.dataset.signature===signature)return;const scroll=e.scrollTop;
    e.replaceChildren(...n.items.map((item,index)=>{const row=element('div','control-list-row');row.classList.toggle('selected',index===n.selected);row.addEventListener('click',()=>control(n.id,'select',{value:index}));
      if(n.kind==='checked-list'){const label=element('label','control-check'),input=element('input');input.type='checkbox';input.checked=!!n.checks[index];input.disabled=!n.enabled;input.addEventListener('change',()=>control(n.id,'item-check',{index,value:input.checked}));label.append(input,document.createTextNode(item));row.append(label);}else text(row,item);return row;}));
    if(!n.items.length){const empty=element('div','control-caption');empty.style.padding='15px';text(empty,'No entries yet.');e.append(empty);}e.dataset.signature=signature;e.scrollTop=scroll;
  }
  function renderTree(parent,list,checks) {
    const signature=JSON.stringify(list);if(parent.dataset.signature===signature)return;const top=parent.scrollTop;
    function branch(n){const wrap=element('div'),row=element('div','tree-row'),expand=element('button');expand.type='button';expand.setAttribute('aria-label',n.expanded?'Collapse':'Expand');expand.disabled=!n.hasChildren;text(expand,n.hasChildren?(n.expanded?'-':'+'):'');expand.addEventListener('click',()=>control(n.id,'expand'));row.append(expand);
      if(checks){const label=element('label','control-check'),input=element('input');input.type='checkbox';input.checked=n.check;input.addEventListener('change',()=>control(n.id,'check',{value:input.checked}));label.append(input,document.createTextNode(n.text));row.append(label);}else{const label=element('span');text(label,n.text);row.append(label);row.addEventListener('click',()=>control(n.id,'select'));}
      wrap.append(row);if(n.expanded){const children=element('div','tree-children');n.nodes.forEach(child=>children.append(branch(child)));wrap.append(children);}return wrap;}
    parent.replaceChildren(...list.map(branch));parent.dataset.signature=signature;parent.scrollTop=top;
  }
  function renderControls(root,tree){appendChildren(root,tree||[]);}
  function wireWindow(){
    document.querySelectorAll('[data-window]').forEach(button=>button.addEventListener('click',()=>send({kind:'window',action:button.dataset.window})));
    ['n','s','e','w','nw','ne','sw','se'].forEach(edge=>{
      const zone=element('div','resize-zone resize-'+edge);zone.setAttribute('aria-hidden','true');document.body.append(zone);let sizing=false;
      const resize=(phase,event)=>send({kind:'resize',phase,edge,x:event.screenX,y:event.screenY});
      zone.addEventListener('pointerdown',event=>{if(event.button!==0)return;event.preventDefault();sizing=true;zone.setPointerCapture(event.pointerId);resize('start',event);});
      zone.addEventListener('pointermove',event=>{if(sizing)resize('move',event);});
      const end=event=>{if(sizing){sizing=false;resize('end',event);}};zone.addEventListener('pointerup',end);zone.addEventListener('pointercancel',end);
    });
    document.addEventListener('keydown',event=>{
      const input=event.target.closest('input[data-control]');
      const dialog=document.querySelector('.dialog-window');
      if(event.key==='Escape'||(event.ctrlKey&&event.key.toLowerCase()==='s')||(event.key==='Enter'&&(input||dialog)&&!event.target.closest('button'))){
        event.preventDefault();if(input)input.dispatchEvent(new Event('change',{bubbles:true}));
        send({kind:'key',id:input?.dataset.control||'',key:event.key,ctrl:event.ctrlKey});
      }
    });
  }
  async function ready(){const emblem=new Image();emblem.src=new URL('../labplus.png',location.href).href;await Promise.all([document.fonts.ready,emblem.decode()]);send({kind:'ready'});}
  function focusContent(){const main=document.querySelector('.research-content,.lr-content')||document.body;main.tabIndex=-1;main.focus({preventScroll:true});}
  let firstFrame;
  function rendered(){
    firstFrame||=(async()=>{focusContent();await document.fonts.ready;await Promise.all([...document.images].map(image=>image.decode()));document.documentElement.getBoundingClientRect();})();
    firstFrame.then(()=>send({kind:'rendered'}));
  }
  globalThis.WorkspaceUI={send,element,text,renderControls,wireWindow,ready,rendered,focusContent};
})();
