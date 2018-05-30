module type ContextConfig = {
  let debugName: string;
  /* the type of data in this context */
  type t;
  /* The current value of the context */
  let value: t;
};

module CreateContext = (C: ContextConfig) => {
  type action =
    | ChangeState(C.t);
  let state = ref(C.value);
  let subscriptions = ref([||]);
  let addSubscription = f => {
    subscriptions := Js.Array.concat([|f|], subscriptions^);
    () => subscriptions := Js.Array.filter(sub => sub !== f, subscriptions^);
  };
  let updateState = newStateOpt => {
    let newState =
      switch (newStateOpt) {
      | None => C.value
      | Some(newValue) => newValue
      };
    state := newState;
    Js.Array.forEach(f => f(newState), subscriptions^);
  };
  module Provider = {
    let component =
      ReasonReact.statelessComponent(C.debugName ++ "ContextProvider");
    let make = (~value=?, ~render, _children) => {
      ...component,
      shouldUpdate: _self => false,
      willReceiveProps: _self => updateState(value),
      didMount: _self => {
        updateState(value);
        ();
      },
      render: _self => render(),
    };
  };
  module Consumer = {
    let component =
      ReasonReact.reducerComponent(C.debugName ++ "ContextConsumer");
    let make = (~render, _children) => {
      ...component,
      initialState: () => state^,
      reducer: (action, _state) =>
        switch (action) {
        | ChangeState(newState) => ReasonReact.Update(newState)
        },
      didMount: ({send, onUnmount}) =>
        (newState => send(ChangeState(newState)))
        |> addSubscription
        |> onUnmount,
      render: ({state}) => render(state),
    };
  };
};
