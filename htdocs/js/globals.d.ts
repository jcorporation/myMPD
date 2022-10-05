// https://code.visualstudio.com/docs/nodejs/working-with-javascript

declare var BSN: object;

interface ChildNode {
    querySelectorAll: any;
    setAttribute: any;
}

interface Element {
    button: any; //custom elements
    name: any;
    style: any;
    value: any;
}

interface EventTarget {
    button: any; //custom elements
    classList: any;
    cloneNode
    getAttribute: any;
    id: any;
    nodeName: any;
    offsetWidth: any;
    parentNode: any;
    remove: any;
    tagName: any;
    value: any;
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
}

interface Navigator {
    userAgentData: any;
    userLanguage: any;
}

interface Node {
    classList: any;
    querySelector: any;
}

interface Object {
    Carousel: any; //BSN
    Collapse: any; //BSN
    Dropdown: any; //BSN
    Modal: any; //BSN
    Popover: any; //BSN
    Toast: any; //BSN
}

interface ParentNode {
    offsetLeft: any;
    scrollTop: any;
}

interface Window {
    trustedTypes: any;
}