# Design Ethos

Random thoughts which I thought it was worth documenting as I'm sure people
will have questions on why I made certain design decisions.

## Why not just handwrite instead of generating? Wouldn't it be less fragile?

Yes to a large extent I agree with this way of thinking. I wrestled a lot with
this before I settled on the design I ended up at. Specifically, 

I went this way for one primary reason: I want this project to function with
minimal maintainence after I consider it "complete". I don't plan on making
many if any changes. I hope this project can go on "autopilot" if I setup enough
autoamtion with minimal intervention required on my part.

The only way to be resilient against future changes that SQLite make to the
grammar upstream. You might be skeptical and think "do they really make changes
I thought SQLite was stable". On the scale of months, you're right. But on the
scale of years (and I've been working on SQLite stuff for 8+ years now), they
can and do major changes to the language.

When this happens, I *don't* want to have to think about how to model all of
that. I just want the autogenerator to take care of that for me and *if*
some human decision making is required (e.g. to decide which nodes to fold)
I want to be able to make that decision within O(minutes).

## Why not fully automate in that case?

The main problem with full automation comes from the AST. By definition there
needs to be some judgement on which nodes can be "folded together": we're not
generating a Concrete Syntax Tree (or Parse Tree) but an abstract one. Which
by definition means somewhere there has to be a call on folding.

I did try and automate that folding to start with but it led to code which
looked like a generic rule on the surface but was so special cased it would
only match exactly one rule. At that point, why not just match the rule directly?

This is why I ended up making the CST -> AST folding specialized and human
written. But I've tried to make it as obvious and easy for any human to
just go ahead and fix divergences with upstream: the tools should tell you
any situation where SQLite has added or removed a rule or token or restructured
their syntax without a matching change on the AST side.
