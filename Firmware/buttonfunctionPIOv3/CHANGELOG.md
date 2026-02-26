# Todo
### Minor
- ...

### Major
- printer error:
    - [E][BLEClient.cpp:238] gattClientEventHandler(): Failed to connect, status=Unknown ESP_ERR error
    - Connected to server
    - (hangs)

### Future Features
- connect only with BLE printer with specific address

### Future Design Changes
- non-BLE printer
- flow-rate sensor or piston pump with stepper
- pump change
- pi-zero base controller instead of comms esp
- io-expander with single esp to control displays and motors


### Outstanding Tests
- pause/continue accuracy
- printer working
- test STATE_CANCEL_IN_DX


## [3.4.0] - 2024-10-15 - Fix report from TK to ML 
### Changed
- statemachine: change _selected_amount_tk --> _selected_amount_ml in "dxi2c.comsOnFinished..."
- printer: 

### Added:
- buttons: add printStreamFiltered -- prints filtered stream for testing purposes.




## [3.3.1] - 2024-09-18
### Changed:
- ESC printer -- small changes

### Added
- (disabled) MutliButtons: AEWMA filter (roll-off could cause false trigger, needs futher work)

## [3.3.0] - 2024-07-05
### Changed
- small clean up


### Added
- device lock feature
- serial: add get string vals if certain command
- device id

## [3.2.1] - 2024-07-05
### Changed
- state machine: add state STATE_CANCEL_IN_DX

## [3.2.0] - 2024-07-05
### Changed
- set voltage cmd parse: fix bug, was sending dx_num,dx_num,low, instead of dx_num, low, high

## [3.1.1] - 2024-07-02
### Changed
- receipt: changed ingredients, product category, add header Urefill and drop "you saved"


# Changelog
## [3.1.0] - 2024-06-26 - Change Log Start
### Changed
- moved thermal printer import to local src folder to remove import issue

### Removed
- config ini: escprinterble dependency 
- config-main: OPT buzzer and printer, changed to NVM config vars

### Added
- statemachine: display update timer object
- statemachine: cfg has printer and buzzer


