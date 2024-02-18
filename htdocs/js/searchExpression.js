"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module searchExpression_js */

//list of search tags that need no operator
/** @type {Array} */
const searchTagsNoOp = [
    'base',
    'modified-since',
    'added-since'
];

//list of search tags that compare against an unix timestamp
/** @type {Array} */
const searchTagsTimestamp = [
    'modified-since',
    'added-since'
];

/**
 * Parses search expressions and update the ui for specified appid
 * @param {string} appid the application id
 * @returns {void}
 */
function handleSearchExpression(appid) {
    const searchStrEl = elGetById(appid + 'SearchStr');
    const searchCrumbEl = elGetById(appid + 'SearchCrumb');
    setFocus(searchStrEl);
    createSearchCrumbs(app.current.search, searchStrEl, searchCrumbEl);
    if (app.current.search === '') {
        searchStrEl.value = '';
    }
    selectTag(appid + 'SearchTags', appid + 'SearchTagsDesc', app.current.filter);
    selectSearchMatch(appid);
}

/**
 * Toggles the state of the SearchMatch select, based on selected tag
 * @param {string} appid the application id
 * @returns {void}
 */
function selectSearchMatch(appid) {
    const searchMatchEl = elGetById(appid + 'SearchMatch');
    //@ts-ignore
    if (searchTagsNoOp.includes(app.current.filter)) {
        elDisable(searchMatchEl);
        searchMatchEl.value = '';
    }
    else {
        elEnable(searchMatchEl);
        if (getSelectValue(searchMatchEl) === undefined) {
            searchMatchEl.value = 'contains';
        }
    }
}

/**
 * Removes the search timer
 * @returns {void}
 */
function clearSearchTimer() {
    if (searchTimer !== null) {
        clearTimeout(searchTimer);
        searchTimer = null;
    }
}

/**
 * Initializes search elements for specified appid
 * @param {string} appid the application id
 * @returns {void}
 */
function initSearchExpression(appid) {
    elGetById(appid + 'SearchTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            app.current.filter = getData(event.target, 'tag');
            selectSearchMatch(appid);
            execSearchExpression(elGetById(appid + 'SearchStr').value);
        }
    }, false);

    elGetById(appid + 'SearchStr').addEventListener('keydown', function(event) {
        //handle Enter key on keydown for IME composing compatibility
        if (event.key !== 'Enter') {
            return;
        }
        clearSearchTimer();
        let value = this.value;
        if (value !== '') {
            const op = getSelectValueId(appid + 'SearchMatch');
            const crumbEl = elGetById(appid + 'SearchCrumb');
            crumbEl.appendChild(createSearchCrumb(app.current.filter, op, value));
            elShow(crumbEl);
            this.value = '';
        }
        if (userAgentData.isAndroid === true) {
            value = '';
        }
        if (value === '') {
            searchTimer = setTimeout(function() {
                execSearchExpression(value);
            }, searchTimerTimeout);
        }
    }, false);

    // Android does not support search on type
    if (userAgentData.isAndroid === false) {
        elGetById(appid + 'SearchStr').addEventListener('keyup', function(event) {
            if (ignoreKeys(event) === true) {
                return;
            }
            //@ts-ignore
            if (searchTagsTimestamp.includes(app.current.filter) &&
                isNaN(parseDateFromText(this.value)) === true)
            {
                // disable search on type for timestamps
                return;
            }
            clearSearchTimer();
            const value = this.value;
            searchTimer = setTimeout(function() {
                execSearchExpression(value);
            }, searchTimerTimeout);
        }, false);
    }

    elGetById(appid + 'SearchCrumb').addEventListener('click', function(event) {
        if (event.target.classList.contains('badge')) {
            //remove search expression
            event.preventDefault();
            event.stopPropagation();
            event.target.parentNode.remove();
            execSearchExpression('');
            elGetById(appid + 'SearchStr').updateBtn();
        }
        else if (event.target.classList.contains('btn')) {
            //edit search expression
            event.preventDefault();
            event.stopPropagation();
            const searchStrEl = elGetById(appid + 'SearchStr');
            searchStrEl.value = unescapeMPD(getData(event.target, 'filter-value'));
            selectTag(appid + 'SearchTags', appid + 'SearchTagsDesc', getData(event.target, 'filter-tag'));
            elGetById(appid + 'SearchMatch').value = getData(event.target, 'filter-op');
            event.target.remove();
            app.current.filter = getData(event.target,'filter-tag');
            execSearchExpression(searchStrEl.value);
            if (elGetById(appid + 'SearchCrumb').childElementCount === 0) {
                elHideId(appid + 'SearchCrumb');
            }
            searchStrEl.updateBtn();
        }
    }, false);

    elGetById(appid + 'SearchMatch').addEventListener('change', function() {
        execSearchExpression(elGetById(appid + 'SearchStr').value);
    }, false);
}

/**
 * Executes the search expression for the current displayed view
 * @param {string} value search value
 * @returns {void}
 */
function execSearchExpression(value) {
    const expression = createSearchExpression(elGetById(app.id + 'SearchCrumb'), app.current.filter, getSelectValueId(app.id + 'SearchMatch'), value);
    appGoto(app.current.card, app.current.tab, app.current.view, 0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, expression, 0);
}

/**
 * Parses a mpd filter expression
 * @param {string} expression mpd filter
 * @returns {object} parsed expression elements or null on error
 */
function parseExpression(expression) {
    if (expression.length === 0) {
        return null;
    }
    let fields = expression.match(/^\((\w+)\s+(\S+)\s+'(.*)'\)$/);
    if (fields !== null &&
        fields.length === 4)
    {
        return {
            'tag': fields[1],
            'op': fields[2],
            'value': unescapeMPD(fields[3])
        };
    }
    // support expressions without operator, e.g. base
    fields = expression.match(/^\(([\w-]+)\s+'(.*)'\)$/);
    if (fields !== null &&
        fields.length === 3)
    {
        return {
            'tag': fields[1],
            'op': '',
            'value': unescapeMPD(fields[2])
        };
    }
    logError('Failure parsing expression: ' + expression);
    return null;
}

/**
 * Creates the search breadcrumbs from a mpd search expression
 * @param {string} searchStr the search expression
 * @param {HTMLElement} searchEl search input element
 * @param {HTMLElement} crumbEl element to add the crumbs
 * @returns {void}
 */
function createSearchCrumbs(searchStr, searchEl, crumbEl) {
    elClear(crumbEl);
    const elements = searchStr.substring(1, app.current.search.length - 1).split(' AND ');
    //add all but last element to crumbs
    for (let i = 0, j = elements.length - 1; i < j; i++) {
        const fields = parseExpression(elements[i]);
        if (fields !== null) {
            crumbEl.appendChild(createSearchCrumb(fields.tag, fields.op, fields.value));
        }
    }
    //check if we should add the last element to the crumbs
    if (searchEl.value === '' &&
        elements.length >= 1)
    {
        const fields = parseExpression(elements[elements.length - 1]);
        if (fields !== null) {
            crumbEl.appendChild(createSearchCrumb(fields.tag, fields.op, fields.value));
        }
    }
    crumbEl.childElementCount > 0
        ? elShow(crumbEl)
        : elHide(crumbEl);
}

/**
 * Creates a search crumb element
 * @param {string} filter the tag
 * @param {string} op search operator
 * @param {string} value filter value
 * @returns {HTMLElement} search crumb element
 */
function createSearchCrumb(filter, op, value) {
    if (op === undefined) {
        op = '';
    }
    const btn = elCreateNodes('div', {"class": ["btn", "btn-dark", "me-2"]}, [
        document.createTextNode(tn(filter) + ' ' + tn(op) + ' \'' + value + '\''),
        elCreateText('div', {"class": ["ml-2", "badge", "bg-secondary", "clickable"]}, '×')
    ]);
    setData(btn, 'filter-tag', filter);
    setData(btn, 'filter-op', op);
    setData(btn, 'filter-value', value);
    return btn;
}

/**
 * Creates a MPD search expression component
 * @param {string} tag tag to search
 * @param {string} op search operator
 * @param {string} value value to search
 * @returns {string} the search expression in parenthesis
 */
function createSearchExpressionComponent(tag, op, value) {
    if (op === 'starts_with' &&
        app.id !== 'BrowseDatabaseList' &&
        features.featStartsWith === false)
    {
        //mpd does not support starts_with, convert it to regex
        if (features.featPcre === true) {
            //regex is supported
            op = '=~';
            value = '^' + value;
        }
        else {
            //best option without starts_with and regex is contains
            op = 'contains';
        }
    }
    //@ts-ignore
    if (searchTagsNoOp.includes(tag)) {
        //this tags needs no operator
        return '(' + tag + ' \'' + escapeMPD(value) + '\')';
    }
    return '(' + tag + ' ' + op + ' ' +
        (op === '>='
            ? value
            : '\'' + escapeMPD(value) + '\''
        ) + ')';
}

/**
 * Creates the MPD search expression from crumbs and parameters
 * @param {HTMLElement} crumbsEl crumbs container element
 * @param {string} tag tag to search
 * @param {string} op search operator
 * @param {string} value value to search
 * @returns {string} the search expression in parenthesis
 */
function createSearchExpression(crumbsEl, tag, op, value) {
    let expression = '(';
    const crumbs = crumbsEl.children;
    for (let i = 0, j = crumbs.length; i < j; i++) {
        if (i > 0) {
            expression += ' AND ';
        }
        expression += createSearchExpressionComponent(
            getData(crumbs[i], 'filter-tag'),
            getData(crumbs[i], 'filter-op'),
            getData(crumbs[i], 'filter-value')
        );
    }
    if (value !== '') {
        if (expression.length > 1) {
            expression += ' AND ';
        }
        expression += createSearchExpressionComponent(tag, op, value);
    }
    expression += ')';
    if (expression.length <= 2) {
        expression = '';
    }
    return expression;
}

/**
 * Creates a mpd filter expression consisting of base and any tag search
 * @param {string} base the base path
 * @param {string} value value to search in any tag
 * @returns {string} the mpd search expression
 */
function createBaseSearchExpression(base, value) {
    let expression = '(base \'' + escapeMPD(base) + '\')';
    if (isEmptyTag(value) === false) {
        expression += ' AND ' + createSearchExpressionComponent('any', 'contains', value);
    }
    return '(' + expression + ')';
}
