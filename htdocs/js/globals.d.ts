// https://code.visualstudio.com/docs/nodejs/working-with-javascript

declare var BSN: object;
declare var i18n: object;

interface ChildNode {
    querySelectorAll: any;
    setAttribute: any;
    style: any;
    title: any;
}

interface Element {
    setValue: any; //custom elements
    addFilterResult: any; //custom elements
    addFilterResultPlain: any; //custom elements
    button: any; //custom elements
    dropdownButton: any; //custom elements
    filterInput: any; //custom elements
    filterResult: any; //custom elements
    updateBtn: any; //custom elements
    name: any;
    offsetHeight: any;
    offsetParent: any;
    offsetTop: any;
    options: any;
    selectedIndex: any;
    style: any;
    value: any;
}

interface Event {
    shiftKey: any;
    ctrlKey: any;
}

interface EventTarget {
    button: any; //custom elements
    classList: any;
    cloneNode
    getAttribute: any;
    id: any;
    nextElementSibling: any;
    previousElementSibling: any;
    nodeName: any;
    offsetWidth: any;
    options: any;
    parentNode: any;
    querySelector: any;
    remove: any;
    selectedIndex: any;
    tagName: any;
    textContent: any;
    value: any;
    closest: any;
}

interface HTMLElement {
    addFilterResult: any; //custom elements
    addFilterResultPlain: any; //custom elements
    checked: any;
    filterInput: any; //custom elements
    filterResult: any; //custom elements
    mozRequestFullScreen: any;
    webkitRequestFullscreen: any;
    msRequestFullscreen: any;
    options: any;
    selectionEnd: any;
    selectedIndex: any;
    selectionStart: any;
    setRangeText: any;
    value: any;
    closest: any;
}

interface Navigator {
    userAgentData: any;
    userLanguage: any;
}

interface Node {
    classList: any;
    querySelector: any;
    removeAttribute: any;
    setAttribute: any;
}

interface Object {
    Carousel: any; //BSN
    Collapse: any; //BSN
    Dropdown: any; //BSN
    Modal: any; //BSN
    Offcanvas: any; //BSN
    Popover: any; //BSN
    Toast: any; //BSN
}

interface ParentNode {
    clientTop: any;
    offsetLeft: any;
    offsetParent: any;
    offsetTop: any;
    scrollTop: any;
    style: any;
}

interface Window {
    trustedTypes: any;
}